#include "quotient_filter_graveyard_hashing.h"
#include <iostream>
#include <fstream>
#include <deque>
#include<stack>

//TODO: CORRECT CLEAN UP CONDITION
//TODO: INCORPORATE CLEAN UP CONDITION FOR EVENLY DISTRIBUTE
//TODO: Try a variant where we walk down to get predecessor and just store predecessor
QuotientFilterGraveyard::QuotientFilterGraveyard(int q, int (*hashFunction)(int), RedistributionPolicy policy=no_redistribution) { //Initialize a table of size 2^(q)
    this->size = 0;
    this->q = q;
    this->hashFunction = hashFunction;
    this->r = sizeof(int)*8 - q; //og : sizeof(int) - q
    this->table_size = (1 << q);
    this->table = (QuotientFilterElement*)calloc(sizeof(QuotientFilterElement), this->table_size);
    this->redistributionPolicy = policy;
    this->opCount=0;
    this->delCount = 0;
    this->REBUILD_WINDOW_SIZE = 0.2*this->table_size; //figure out good numerical value for this based on quotient filter paper
}

QuotientFilterGraveyard::~QuotientFilterGraveyard() {
    free(this->table);
}

void QuotientFilterGraveyard::insertElement(int value) {
    // make sure table isn't full
    if (this->size == this->table_size) {
        return;
    }

    FingerprintPair f = fingerprintQuotient(value);
    bool originally_occupied = table[f.fq].is_occupied;

    // set occupied bit to 1
    table[f.fq].is_occupied = 1;

    // if there's a valid tombstone for a new run:
    int target_slot = f.fq;
    if (table[target_slot].isTombstone) {

        PredSucPair ps = decodeValue(table[target_slot].value);
        int pred = ps.predecessor;
        int succ = ps.successor;

        bool valid_for_insertion;
        if (succ > pred) {
            valid_for_insertion = pred <= f.fq && f.fq < succ;
        }
        else if (succ == pred) {
            // case of no successor
            valid_for_insertion = true;
        }
        else {
            // accounting for the case of wrapping around the end of the array
            valid_for_insertion = (pred <= f.fq && f.fq < this->table_size) ||
                                  (0 < f.fq && f.fq < succ);
        }
        // valid_for_insertion |= table[target_slot].isEndOfCluster;
        if (valid_for_insertion) {
            table[target_slot].isTombstone = false;
            table[target_slot].value = f.fr;
            table[target_slot].is_continuation = 0;
            table[target_slot].is_shifted = 0;
            this->size += 1;

            updateAdjacentTombstonesInsert(f.fq);
            return;
        }
    }

    // otherwise:
    // Find the beginning of the run
    // std::cout << "Here 0\n";
    target_slot = findRunStartForBucket(f.fq, true);

    // Scan to end of run
    if (originally_occupied) {
        do {
            target_slot = (target_slot + 1) % table_size;
        } while (table[target_slot].is_continuation && !table[target_slot].isTombstone);
    }

    // std::cout << "Here\n";
    // shift following runs (Edit: only do this if element was not put in an empty slot)
    if (!isEmptySlot(target_slot, f.fq)) {
        shiftElementsUp(target_slot, f.fq);
    }
    // std::cout << "Here 2\n";
    if (!originally_occupied) {
        updateAdjacentTombstonesInsert(target_slot);
    }
    // std::cout << "Here 3\n";
    // insert element
    table[target_slot].isTombstone = false;
    table[target_slot].value = f.fr;
    table[target_slot].is_continuation = originally_occupied;
    table[target_slot].is_shifted = (target_slot != f.fq);
    this->size += 1;

    //Perform redistribution if necessary
    this->opCount +=1;
    // std::cout << "OVER HERE\n";
    redistribute();
    // std::cout << "REDISTRIBUTE DONE HERE\n";

}

bool QuotientFilterGraveyard::isEmptySlot(int slot, int bucket) {
    bool original_occupation;
    if (bucket==slot){
        original_occupation = false;
    } else{
        original_occupation = table[slot].is_occupied;
    }
    return (!table[slot].is_shifted && !original_occupation)||table[slot].isTombstone;
}

void QuotientFilterGraveyard::deleteElement(int value) {
    //Step 1: Figure out value finger print
    FingerprintPair f = fingerprintQuotient(value);

    //Step 2: If that location's fingerprint is occupied, insert tombstone and keep moving
    if (table[f.fq].is_occupied) {
        //Find predecessor value and position of element
        PairVal res;
        findRunStartForBucket(f.fq, &res);
        // int s = res.runStart;
        int startOfRun = res.runStart;
        int deletePointIndex;
        int successorBucket = findNextBucket(f.fq, startOfRun);
        bool found = false;
        do {
            if (table[startOfRun].value == f.fr) {
                deletePointIndex = (startOfRun+1)%table_size;
                found = true;
            }
            if (table[startOfRun].is_occupied && successorBucket == f.fq) { //set this once
                successorBucket = startOfRun;
            }
            startOfRun = (startOfRun+1)%table_size;
        }
        while (table[startOfRun].is_continuation && !found);
        //Check if the element is found before inserting tombstone at the end of run
        if (found) {
            int nextItem = (res.runStart + 1)%table_size;
            if (!table[nextItem].is_continuation|| table[nextItem].isTombstone) { //Set is_occupied accurately
                table[f.fq].is_occupied = false;
                //If run was shifted, it may affect the tombstones in the run where its actual bucket is located
                if (table[res.runStart].is_shifted) {
                    resetTombstoneSuccessors(f.fq);
                }
            }
            shiftTombstoneDown(deletePointIndex, f.fq, successorBucket, res);
            this->size--;
            this->delCount++;
        }
        //Cleanup tombstones if tombstones take over 30% of the table
        if (redistributionPolicy == between_runs || redistributionPolicy == between_runs_insert || redistributionPolicy == evenly_distribute) {
            float tombStoneRatio = this->delCount/(float)this->table_size;
            if (tombStoneRatio > 0.3) {
                cleanUpTombstones();
            }
        }
    }
}

void QuotientFilterGraveyard::resetTombstoneSuccessors(int bucket) {

    //Step 1: Scan forward to find new successor
    //At the same  time, we want to also find the start of the tombstones
    int startOfTombstones;
    bool tombstoneFound = false;
    bool newSuccessorFound = false;
    int curr = bucket;
    int successor = bucket;
    while (table[curr].is_continuation) {
        if (!newSuccessorFound && table[curr].is_occupied) {
            successor = curr;
            newSuccessorFound = true;
        }
        if (!tombstoneFound && table[curr].isTombstone) {
            startOfTombstones = curr;
            tombstoneFound = true;
        }
        if (newSuccessorFound && tombstoneFound) {
            break;
        }
        curr = (curr-1 + table_size)%table_size;
    }

    //Step 2: If tombstone found, reset their values
    if (tombstoneFound) {
        PredSucPair predsuc = decodeValue(table[startOfTombstones].value);
        int currTombstone = startOfTombstones;
        while (table[currTombstone].is_continuation) {
            if (newSuccessorFound) {
                table[currTombstone].value = encodeValue(predsuc.predecessor, successor);
            } else { //If end of cluster, value unnecessary
              table[currTombstone].value = encodeValue(predsuc.predecessor, predsuc.predecessor);  
            }
            currTombstone = (currTombstone + 1)%table_size;
        }
    }

}

void QuotientFilterGraveyard::shiftTombstoneDown(int afterTombstoneLocation, int tombstonePrededcessorBucket, int tombstoneSuccessorBucket, PairVal cleanUpInfo) {
    int currPointer =  afterTombstoneLocation;
    int prevPointer = (currPointer - 1 + table_size)%table_size;
    int successorBucket = tombstoneSuccessorBucket;
    //If cleanup, just move the elements up and leave the tombstones
    if (cleanUpInfo.cleanUpNeeded && redistributionPolicy == amortized_clean) { 
        int startOfReading = cleanUpInfo.runStart;
        int startOfWriting = cleanUpInfo.tombstoneStart;
        while (startOfReading != prevPointer && !table[startOfReading].isTombstone) {
            // std::cout << "Start of Reading: " << startOfReading << " Start of Writing: " << startOfWriting << "\n";
            bool is_occupiedBitBefore = table[startOfWriting].is_occupied;
            table[startOfWriting] = table[startOfReading];
            table[startOfWriting].is_occupied = is_occupiedBitBefore;
            table[startOfWriting].is_shifted = !(tombstonePrededcessorBucket == startOfWriting);
            startOfReading = (startOfReading + 1)%table_size;
            startOfWriting = (startOfWriting + 1)%table_size;
        }
    }
    //Push tombstone to the end of the run  or to the start of  a stretch of tombstonese
    while (table[currPointer].is_continuation && !table[currPointer].isTombstone) {

        table[prevPointer].value = table[currPointer].value;
        if (table[currPointer].is_occupied && successorBucket == tombstonePrededcessorBucket) { //Finish walking down the run and update successorBucket once
            successorBucket = currPointer;
        }
        table[prevPointer].is_shifted = !(prevPointer == tombstonePrededcessorBucket);
        prevPointer = currPointer;
        currPointer = (currPointer + 1)%table_size;
    }
    //Set successor appropriately if run contains all tombstones
    if (table[currPointer].isTombstone && table[currPointer].is_continuation) {
        PredSucPair res = decodeValue(table[currPointer].value);
        successorBucket = res.successor;
    }
    table[prevPointer].value = encodeValue(tombstonePrededcessorBucket, successorBucket);
    table[prevPointer].isTombstone = true;

}

long long int QuotientFilterGraveyard::encodeValue(int predecessor, int successor) {
    long long int result = predecessor; //Put predecessor in lower bits
    result = result << sizeof(int)*8; //shift it to the top 32 bits
    long long int final = result|successor;
    return result|successor; //Or the shifted predecessor with the successor
}

PredSucPair QuotientFilterGraveyard::decodeValue(long long value){
    PredSucPair res;
    res.predecessor= value >> sizeof(int)*8;
    res.successor = value;
    return res;
}

// Updates the predecessors and successors of tombstones adjacent to a newly created run.
void QuotientFilterGraveyard::updateAdjacentTombstonesInsert(int newRun) {
    int target_slot = (newRun + 1) % table_size;
    while (table[target_slot].isTombstone) {
        PredSucPair oldPs = decodeValue(table[target_slot].value);
        table[target_slot].value = encodeValue(newRun, oldPs.successor);
        target_slot = (target_slot + 1) % this->table_size;
    }

    target_slot = (newRun - 1 + table_size) % table_size;
    while (table[target_slot].isTombstone) {
        PredSucPair oldPs = decodeValue(table[target_slot].value);
        table[target_slot].value = encodeValue(oldPs.predecessor, newRun);
        target_slot = (target_slot - 1 + table_size) % table_size;
    }
}

/* Helper function; assumes that the target bucket is occupied
*/
void QuotientFilterGraveyard::findRunStartForBucket(int target_bucket, PairVal* res) {
    this->findRunStartForBucket(target_bucket, false, res);
}

int QuotientFilterGraveyard::findRunStartForBucket(int target_bucket) {
    return this->findRunStartForBucket(target_bucket, false);
}

int QuotientFilterGraveyard::findRunStartForBucket(int target_bucket, bool stop_at_tombstones){
    // Check for item in table
    if (!table[target_bucket].is_occupied) {
        return -1;
    }
    // Backtrack to beginning of cluster
    int bucket = target_bucket;
    while (table[bucket].is_shifted) {
        bucket = (bucket - 1 + table_size) % table_size;
    }

    // Find the run for fq
    int run_start = bucket;
    int next_bucket = bucket;
    while (bucket != target_bucket) {
        // Find the next bucket
        do {
            next_bucket = (next_bucket + 1) % table_size;
        }
        while (!table[next_bucket].is_occupied);

        // Skip to the next run
        if (!(stop_at_tombstones && next_bucket == target_bucket)) {
            do {
                run_start = (run_start + 1) % table_size;
            }
            while (table[run_start].is_continuation || table[run_start].isTombstone);
        }
        // If permitted, can stop at a tombstone of the predecessor run
        else if (!table[run_start].isTombstone) {
            do {
                run_start = (run_start + 1) % table_size;
            }
            while (table[run_start].is_continuation && !table[run_start].isTombstone);
        }

        bucket = next_bucket;
    }
    return run_start;
}

int QuotientFilterGraveyard::findRunStartForBucket(int target_bucket, bool stop_at_tombstones, PairVal* res){
    // Check for item in table
    if (!table[target_bucket].is_occupied) {
        return -1;
    }
    // Backtrack to beginning of cluster
    int bucket = target_bucket;
    int numTombstones = 0;
    while (table[bucket].is_shifted) {
        if (table[bucket].isTombstone) {
            numTombstones++;
        }
        if (numTombstones == 1) {
            res->tombstoneStart = bucket;
        }
        bucket = (bucket - 1 + table_size) % table_size;
    }

    //Check if any cleanup needs to happen
    if (numTombstones > 0) {
        res->cleanUpNeeded = true;
    } else{
        res->cleanUpNeeded = false;
    }

    // Find the run for fq
    int run_start = bucket;
    int next_bucket = bucket;
    while (bucket != target_bucket) {
        // Find the next bucket
        do {
            next_bucket = (next_bucket + 1) % table_size;
        }
        while (!table[next_bucket].is_occupied);

        // Skip to the next run
        if (!(stop_at_tombstones && next_bucket == target_bucket)) {
            do {
                run_start = (run_start + 1) % table_size;
            }
            while (table[run_start].is_continuation || table[run_start].isTombstone);
        }
        // If permitted, can stop at a tombstone of the predecessor run
        else if (!table[run_start].isTombstone) {
            do {
                run_start = (run_start + 1) % table_size;
            }
            while (table[run_start].is_continuation && !table[run_start].isTombstone);
        }

        bucket = next_bucket;
    }
    res->runStart =  run_start;
    return run_start;
}

/* Returns the first slot after the cluster containing the given slot
 * (i.e. guaranteed to return a different slot, assuming the table isn't full)
*/
int QuotientFilterGraveyard::findEndOfCluster(int slot) {
    int current_slot = slot;
    do {
        // std::cout << "Current Slot" << current_slot << "\n";
        current_slot = (current_slot + 1) % table_size;
    }
    while (table[current_slot].is_shifted && !table[current_slot].isTombstone);

    return current_slot;
}

void QuotientFilterGraveyard::shiftElementsUp(int start, int bucketNumber) {
    int end = findEndOfCluster(start);
    // Make sure the end of one cluster isn't the start of another
    // int count = 0;

    while (!isEmptySlot(end,bucketNumber)) {
        // count++;
        // if (count < 10) {
        //     std::cout << "End: " << end << "\n";
        //     std::cout << "ISTOMBSTONE: " << table[end].isTombstone << "\n";
        //     std::cout << "ISOCCUPIED: " << table[end].is_occupied << "\n";
        //     std::cout << "ISSHIFTED: " << table[end].is_shifted << "\n";
        //     std::cout << "ISCONTINUATION: " << table[end].is_shifted << "\n";
        // }
        end = findEndOfCluster(end);
    }

    // end should now point to an open position
    while (end != start) { //Corner case: End is an empty slot next to start.
        // std::cout << "HERE\n";
        int target = (end - 1 + table_size) % table_size;
        table[end].isTombstone = table[target].isTombstone;
        table[end].value = table[target].value;
        table[end].is_continuation = table[target].is_continuation;
        table[end].is_shifted = true;
        end = target;
    }
    // while (!table[*b].is_occupied);
}


bool QuotientFilterGraveyard::query(int value) {
    FingerprintPair f = fingerprintQuotient(value);
    if (table[f.fq].is_occupied) {
        if (table[f.fq].value == f.fr) {
            return true;
        } else {
            PairVal res;
            findRunStartForBucket(f.fq, &res);
            int s = res.runStart;
            //Once you locate the run containing item, look through to find it
            if (res.cleanUpNeeded && redistributionPolicy == amortized_clean) {
                return cleanAndSearch(res, f.fq, f.fr);
            } else {
                do {
                    if (table[s].value == f.fr) {
                        return true;
                    }
                    s = (s+1)%table_size;
                }
                while (table[s].is_continuation && !table[s].isTombstone); //Stop when you see a tombstone
            }

        }
    }
    return false;
}

bool QuotientFilterGraveyard::cleanAndSearch(PairVal cleanUpInfo, int bucketNumber, int remainderToFind) {
        int startOfReading = cleanUpInfo.runStart;
        int startOfWriting = cleanUpInfo.tombstoneStart;
        bool found = false;
        while (table[startOfReading].is_continuation && !table[startOfReading].isTombstone) {
            // std::cout << "Start of Reading: " << startOfReading << " Start of Writing: " << startOfWriting << "\n";
            bool is_occupiedBitBefore = table[startOfWriting].is_occupied;
            table[startOfWriting] = table[startOfReading];
            table[startOfWriting].is_occupied = is_occupiedBitBefore;
            table[startOfWriting].is_shifted = !(bucketNumber == startOfWriting);
            startOfReading = (startOfReading + 1)%table_size;
            startOfWriting = (startOfWriting + 1)%table_size;
            if (table[startOfReading].value == remainderToFind) {
                found = true;
            }
        }
        return found;
}

FingerprintPair QuotientFilterGraveyard::fingerprintQuotient(int value) {
    uint32_t hash = this->hashFunction(value);
    FingerprintPair res;
    res.fq = hash >> this->r;
    res.fr = hash % (1 << this->r);
    return res;
}

int QuotientFilterGraveyard::startOfCopy(int start){
    while (table[start].isTombstone && table[start].is_shifted) {
        start = (start + 1)%table_size;
    }
    return start;
}

int QuotientFilterGraveyard::startOfWrite(int start) {
    while (!table[start].isTombstone && table[start].is_shifted) {
        start = (start + 1)%table_size;
    }
    if (table[start].isTombstone && table[start].is_shifted) { //If we ended because we found a tombstone, leave one and start writing after it
        start = (start + 1)%table_size;
    }
    return start;
}

/**
 * Given an index, returns the index where the next cluster starts
 * */
int QuotientFilterGraveyard::findClusterStart(int pos) {
    int start = pos;
    do {
        //walk backwards to start of cluster if one exists
        while(table[start].is_shifted) {
            start = (start - 1 + table_size)%table_size;
        }
        // std::cout << "Ended with start: " << start << "\n";
        //At this point start, if it is truly an item, is pointing to the start of some cluster
        if (table[start].isTombstone || table[start].is_occupied){
            return start;
        } else {
            start = (start - 1 + table_size) % table_size; //otherwise, search for the cluster elsewhere;
        }
    } while(start != pos);
    return start;
}

//The goal is to advance the position to write to be the same as the bucket to be copied
int QuotientFilterGraveyard::correctStartOfWrite(int startOfWriting, int toBeCopiedBucket) {
    int initialStartOfWriting = startOfWriting;
    while (startOfWriting != toBeCopiedBucket && table[startOfWriting].is_shifted && table[startOfWriting].isTombstone) { //make sure you are still in the cluster!
        startOfWriting = (startOfWriting + 1)%table_size;
    }
    if (table[startOfWriting].isTombstone) {
        return startOfWriting;
    } else {
        return initialStartOfWriting;
    }
}

Res QuotientFilterGraveyard::findStartOfWriteAndCopy(int startOfCluster) {
    int startOfCopying = startOfCluster;
    int startOfWriting = startOfCluster;
    Res res;
    res.val4 = normal_case;
    int currBucket;
    // std::cout << "startOfWriting: " << startOfWriting << " startOfCopying: " << startOfCopying << "\n";
    if (table[startOfWriting].isTombstone || (!table[startOfWriting].is_continuation && !table[startOfWriting].is_shifted && !table[startOfWriting].is_occupied)) { //If starts with tombstone or empty spot
        startOfCopying = startOfCopy((startOfCopying + 1)%table_size);
        if (table[startOfCopying].is_shifted) { 
            PredSucPair predsuc  = decodeValue(table[startOfWriting].value);
            currBucket = predsuc.successor;
            startOfWriting = correctStartOfWrite((startOfWriting+1)%table_size, currBucket);
        }  else{ //Cluster has  all tombstones so no things to copy over
            res.val4 = all_tombstones;
        }
    } else {
        startOfWriting = startOfWrite((startOfWriting+1)%table_size);
        if (table[startOfWriting].is_shifted) {
            startOfCopying = startOfCopy(startOfWriting);
            if (!table[startOfCopying].is_shifted) { //If we don't have anything to move up in the cluster
                int start = (startOfWriting + 1) % table_size;
                while (start != startOfCopying) { //Empty out the tombstones after leaving just one
                    table[start].is_continuation = false;
                    table[start].isTombstone = false;
                    table[start].is_occupied = false;
                    table[start].is_shifted = false;
                    start = (start + 1) % table_size;
                }
                res.val4 = nothing_to_push;
            } else {
                int tombstoneBefore = (startOfCopying-1+table_size)%table_size;
                PredSucPair predsuc  = decodeValue(table[tombstoneBefore].value);
                currBucket = predsuc.successor;
                startOfWriting = correctStartOfWrite(startOfWriting, currBucket);
            }
        } else{ 
            res.val4 = no_tombstones;
        }
    }   
    
    res.val1 = startOfCopying;
    res.val2 = startOfWriting;
    res.val3 = currBucket;
    return res;
}

/**
 * Reorganizes the a cluster by moving down elements in runs down if there are available tombstones
 * */
int QuotientFilterGraveyard::reorganizeCluster(int startOfCluster){
    // std::cout << "Reorganizing cluster starting at " << startOfCluster << "\n";
    Res res = findStartOfWriteAndCopy(startOfCluster);
    switch(res.val4) {
        case all_tombstones:
            return res.val1; //If all tombstones, then where we start copying from is the end
        case no_tombstones:
        case nothing_to_push:
            return res.val2;
        default:
            return shiftClusterElementsDown(res, false);
    }
}

int QuotientFilterGraveyard::shiftClusterElementsDown(Res res, bool noleave_tombstone=false) {
    int startOfCopying = res.val1;
    int startOfWriting = res.val2;
    int currBucket = res.val3;
    if (startOfCopying != startOfWriting) {
        int startBucket = currBucket;
        do { //Copy all runs, separated by a tombstone where possible if noleave is false
            //Set start bucket appropriately
            int startOfWritingInitial = startOfWriting;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
            do {
                //Copy over element, maintaining is_occupied bit
                bool initialOccupiedBit = table[startOfWriting].is_occupied;
                table[startOfWriting] = table[startOfCopying];
                table[startOfWriting].is_occupied = initialOccupiedBit;
                table[startOfWriting].is_shifted = !(startOfWriting == startBucket);
                table[startOfWriting].isTombstone = false;
                //Zero out copied element
                table[startOfCopying].isTombstone = true;

                //Advance pointers and get bucket of next run if not already got
                startOfWriting = (startOfWriting + 1) % table_size;
                startOfCopying = (startOfCopying + 1) % table_size;
                if (table[startOfWriting].is_occupied && currBucket == startBucket){
                    currBucket = startOfWriting;
                }
            }
            while(!table[startOfCopying].isTombstone && table[startOfCopying].is_continuation && startOfCopying!=startOfWritingInitial);  //Make sure we don't place more tombstones than we have room for

            //Check to see if we still have room to write
            if (startOfCopying == startOfWriting){
                break;
            }
            //Add a tombstone after the run to break up cluster.
            if (!noleave_tombstone) {
                table[startOfWriting].isTombstone = true;
                startOfWriting = (startOfWriting + 1) % table_size;
            }

            //Case 1: You hit a tombstone while copying
            if (table[startOfCopying].isTombstone) {
                PredSucPair predsuc  = decodeValue(table[startOfCopying].value);
                startOfWriting = correctStartOfWrite(startOfWriting, predsuc.successor);
                startOfCopying = startOfCopy(startOfCopying);
                currBucket = predsuc.successor;
            }
            //Case 2: You hit a new run while copying
            if (!table[startOfCopying].is_continuation) {
                if (table[startOfWriting].is_occupied && currBucket==startBucket) {
                    currBucket = startOfWriting;
                }
            }
            startBucket = currBucket;
            // std::cout << "startOfWriting:" <<  startOfWriting << "startOfCopying: " << startOfCopying <<"\n"; 
        } while (table[startOfCopying].is_shifted); //do this process while we are still within the cluster

        //Empty out the tombstones after leaving just one
        if (startOfWriting != startOfCopying) {
            int startOfWriting = (startOfWriting + 1) % table_size;
            while (startOfWriting != startOfCopying) { 
                table[startOfWriting].is_continuation = false;
                table[startOfWriting].isTombstone = false;
                table[startOfWriting].is_occupied = false;
                table[startOfWriting].is_shifted = false;
                startOfWriting = (startOfWriting + 1) % table_size;
            }
        }
    }
    return startOfCopying;
}

int QuotientFilterGraveyard::findEndOfRun(int startOfRun, int * predecessor, int * successor) { //Function assumes we have no tombstones in the run
    Pair res;
    bool found = false;
    int endOfRun = startOfRun;
    do {
        endOfRun = (endOfRun + 1) % table_size;
        if (table[endOfRun].is_occupied && !found) {
            *successor = endOfRun;
            found = true;
        }
    } while (table[endOfRun].is_continuation && table[endOfRun].is_shifted);

    while (table[startOfRun].is_continuation) {
        startOfRun = (startOfRun - 1 + table_size) % table_size;
    }
    *predecessor = startOfRun;
    return endOfRun;
}

Opt QuotientFilterGraveyard::separateRunsByTombstones(int startOfCluster) {
    Opt opt;
    //Find end of run and push onto queue
    int predecessor;
    int successor;
    int runEnd = findEndOfRun(startOfCluster, &predecessor, &successor); //Really the next thing after the run ends
    if (runEnd == startOfCluster) { //Do not do anything because the table is full!
        opt.val2 = true;
        opt.val1 = runEnd;
        return opt;
    }
    int nextSlot = runEnd;
    std::deque<QuotientFilterElement> backedUpElements;
    std::stack<int> lastDisplaced;
    backedUpElements.push_back(table[nextSlot]);
    lastDisplaced.push(nextSlot);
    //Calculate the bucket of run
    int prevBucket = predecessor;
    int currBucket = successor;
    do {
        //Place tombstone at the next slot and push index to stack
        table[nextSlot].isTombstone = true;
        table[nextSlot].value = encodeValue(prevBucket, currBucket); //the tombstone is for the preceeding run
        table[nextSlot].is_continuation = true;
        table[nextSlot].is_shifted = true;
        // std::cout << "NEW RUN\n";
        //Shift items from the queue into the nextSlot until you hit the end of a run
        int nextBucket = currBucket;
        bool nextBucketFound = false;
        do {
            //pop element to place down
            QuotientFilterElement currElement = backedUpElements.front();
            backedUpElements.pop_front();

            //Element may be empty slot
            if (currElement.isTombstone|| (!currElement.is_occupied && !currElement.is_shifted)){
                opt.val2 = true;
                opt.val1 = nextSlot;
                return opt;
            } else { //element is an actual value
                //Create room to place element
                nextSlot = (nextSlot+1)%table_size;
                //Keep track of next successor
                if (table[nextSlot].is_occupied && !nextBucketFound) {
                    nextBucket = nextSlot;
                    nextBucketFound = true;
                }
                //If you wrap around, undo all placements up until necessary and return
                if (nextSlot == runEnd){
                    //Place elements back in their original position
                    backedUpElements.push_front(currElement);
                    while (!backedUpElements.empty()) {
                        int index = lastDisplaced.top();
                        lastDisplaced.pop();
                        if (!table[index].isTombstone){
                            backedUpElements.push_front(table[index]);
                        }
                        table[index] = backedUpElements.back();
                        backedUpElements.pop_back();
                    }
                    opt.val2 = true;
                    opt.val1 = (startOfCluster - 1 + table_size)%table_size; //set equal to right before cluster so we break insertion of tombstones
                    return opt;
                }
                backedUpElements.push_back(table[nextSlot]);
                lastDisplaced.push(nextSlot);
                //Place element making sure to maintain is_occupied bit
                bool initialIsOccupied = table[nextSlot].is_occupied;
                table[nextSlot] = currElement;
                table[nextSlot].is_occupied = initialIsOccupied; //is_occupied bit is independent of run, so do not change!
                table[nextSlot].is_shifted = true; //clusters that were once separate are now conjoined so merge them
            }
        }while (backedUpElements.front().is_continuation && !backedUpElements.front().isTombstone);//Keep putting elements down until we put down a run
        //Make allowance for tombstone at the end of the run
        nextSlot = (nextSlot+1)%table_size;
        backedUpElements.push_back(table[nextSlot]);
        lastDisplaced.push(nextSlot);
        prevBucket = currBucket;
        currBucket = nextBucketFound;
    } while (true);
}

int QuotientFilterGraveyard::findNextBucket(int bucketNum, int endIndex) {
    if (endIndex == -1) {
        do {
            bucketNum = (bucketNum + 1)% table_size;
        }
        while(!table[bucketNum].is_occupied); //do-while because we start at a bucket number
    } else {
        int start = bucketNum;
        do {
            start = (start + 1)% table_size;
        }
        while(!table[bucketNum].is_occupied && bucketNum < endIndex);
        if (table[start].is_occupied) {
            bucketNum = start; //Only reset if you found a new bucket
        }
    }
    return bucketNum;
}

int QuotientFilterGraveyard::reorganizeCluster2(int startOfCluster){
    //Start by trying to reorganize as in policy1
    Res res = findStartOfWriteAndCopy(startOfCluster);
    switch(res.val4) {
        case all_tombstones:
            return res.val1;
        case nothing_to_push:
            return res.val2;
        case no_tombstones:
            return separateRunsByTombstones(startOfCluster).val1;
        default:
            int initialRes = shiftClusterElementsDown(res);
            if (table[initialRes].is_shifted){ //If we are still within the cluster, then we continue processing
                return separateRunsByTombstones(initialRes).val1;
            } else{
                return initialRes;
            }
    }
}

int QuotientFilterGraveyard::findStartOfTombstonesInRun(int pos){
    do {
        pos = (pos + 1)%table_size;
    } while(table[pos].is_continuation && !table[pos].isTombstone);
    return pos;
}

bool QuotientFilterGraveyard::insertTombstone(int pos) {
    //Step 1: Check if position is empty
    if (!table[pos].is_occupied && !table[pos].is_shifted) {
        table[pos].isTombstone = true;
        table[pos].is_continuation = true;
        table[pos].value = encodeValue(pos,pos);
        return true;
    } else if (table[pos].isTombstone) {
        return true;
    }else  {
        int successor;
        int predecessor;
        int endOfRun = findEndOfRun(pos, &predecessor, &successor);
        //Case 1: empty
        if (!table[endOfRun].is_occupied && !table[endOfRun].is_shifted) {
            table[endOfRun].isTombstone = true;
            table[endOfRun].is_continuation = true;
            table[endOfRun].is_shifted = true;
            table[endOfRun].value = encodeValue(predecessor, successor);
            return true;
        } else if (table[endOfRun].isTombstone) {
            return true;
        } else {
            // std::cout << "End of run: " << endOfRun << "\n";
            std::deque<QuotientFilterElement> replacedElements;
            std::stack<int> lastDisplaced;
            QuotientFilterElement replacedElement = table[endOfRun];
            lastDisplaced.push(endOfRun);
            replacedElements.push_back(replacedElement);

            //Put tombstone at end of run
            table[endOfRun].isTombstone = true;
            table[endOfRun].is_continuation = true;
            table[endOfRun].value = encodeValue(predecessor, successor); 
            table[endOfRun].is_shifted = true;

            int start = (endOfRun+1)%table_size;
            QuotientFilterElement temp = table[start];
            //until the replaced element is empty or a tombstone, keep shifting element down
            while (!(replacedElement.isTombstone || (!replacedElement.is_occupied && !replacedElement.is_shifted))){
                // std::cout << "Start: " << start << "\n";
                if (start == endOfRun) { //If we wrap around because we have no more room for tombstones, undo everything
                    // std::cout << "In here\n";
                    while (!replacedElements.empty()) {
                        int index = lastDisplaced.top();
                        lastDisplaced.pop();
                        if (!table[index].isTombstone){
                            replacedElements.push_front(table[index]);
                        }
                        table[index] = replacedElements.back();
                        replacedElements.pop_back();
                    }
                    return false; //Tell redistribute func to terminate early!
                }
                //Pop element from the front
                replacedElement = replacedElements.front();
                replacedElements.pop_front();

                temp = table[start];
                table[start] = replacedElement;
                table[start].is_occupied = temp.is_occupied; //maintain is_occupied bit despite shift
                //Correctly set is_shifted bit
                // int prevStart = (start - 1 + table_size)%table_size;
                // table[start].is_shifted = !(prevStart == start);
                table[start].is_shifted = true; //We necessarily remove you from your original place!
                lastDisplaced.push(start);
                replacedElements.push_back(temp);
                start = (start + 1) % table_size; 
        }
        return true;
    }

    // //Step 1: Check if position is either a tombstone or empty before doing any work
    // if (table[pos].isTombstone) {
    //     std::cout <<"HERE\n";
    //     return true;
    // } else if (!table[pos].is_occupied && !table[pos].is_shifted) {
    //     table[pos].isTombstone = true;
    //     table[pos].is_continuation = true;
    //     table[pos].value = encodeValue(pos,pos);
    //     // table[pos].isEndOfCluster = true;
    //     return true;
    // } else { //Step 2: Otherwise, we must go to the end of the run
    //     // std::cout << "POSITION OCCUPIED\n";
    //     int endOfRun = findStartOfTombstonesInRun(pos); //next position after just ended
    //     // std::cout << "end of run starting at " << pos << ": " << endOfRun << "\n";
    //     //Slot may be empty, a tombstone or an element
    //     if (!table[endOfRun].is_occupied && !table[endOfRun].is_shifted) { //Case 1: empty
    //         table[endOfRun].isTombstone = true;
    //         table[endOfRun].is_continuation = true;
    //         // table[endOfRun].isEndOfCluster = true;
    //         table[endOfRun].is_shifted = true;
    //         return true;
    //     } else if (table[endOfRun].isTombstone) { //Case 2: tombstone
    //         // std::cout << "POSITION IS A TOMBSTONE\n";
    //         return true;
    //     } else { //begin process of shifting elements down after inserting tombstone
    //         //Put replaced element in Queue
    //         std::deque<QuotientFilterElement> replacedElements;
    //         std::stack<int> lastDisplaced;
    //         QuotientFilterElement replacedElement = table[endOfRun];
    //         lastDisplaced.push(endOfRun);
    //         replacedElements.push_back(replacedElement);

    //         //Put tombstone at end of run
    //         table[endOfRun].isTombstone = true;
    //         table[endOfRun].is_continuation = true;
    //         table[endOfRun].value = 
    //         // table[endOfRun].isEndOfCluster = true;
    //         table[endOfRun].is_shifted = true;

    //         int start = endOfRun+1;
    //         QuotientFilterElement temp = table[start];
    //         //until the replaced element is empty, keep shifting element down
    //         while (!(!replacedElement.is_occupied && !replacedElement.is_shifted)){

    //             if (start == pos) { //If we wrap around because we have no more room for tombstones, undo everything
    //                 while (!replacedElements.empty()) {
    //                     int index = lastDisplaced.top();
    //                     lastDisplaced.pop();
    //                     if (!table[index].isTombstone){
    //                         replacedElements.push_front(table[index]);
    //                     }
    //                     table[index] = replacedElements.back();
    //                     replacedElements.pop_back();
    //                 }
    //                 return false; //Tell redistribute func to terminate early!
    //             }
    //             //Pop element from the front
    //             replacedElement = replacedElements.front();
    //             replacedElements.pop_front();

    //             temp = table[start];
    //             table[start] = replacedElement;
    //             table[start].is_occupied = temp.is_occupied; //maintain is_occupied bit despite shift
    //             //Correctly set is_shifted bit
    //             int prevStart = (start - 1 + table_size)%table_size;
    //             table[start].is_shifted = !(prevStart == start);
    //             lastDisplaced.push(start);
    //             replacedElements.push_back(temp);
    //             start = (start + 1) % table_size;
    //         }
            
    //         return true;
    //     }
    }
}

int QuotientFilterGraveyard::cleanUpHelper(int clusterStart) {
    Res res = findStartOfWriteAndCopy(clusterStart);
    switch(res.val4) {
        case normal_case:
            return shiftClusterElementsDown(res, true);
        case all_tombstones:
            return res.val1;
        default:
            return res.val2;
    }   
}

void QuotientFilterGraveyard::cleanUpTombstones(){ 
    //Clean up tombstones in table when 30% of table overrun by tombstones
        int initialStart = findClusterStart(0);
        int currCluster = initialStart; 
        do {
            int endOfCluster = cleanUpHelper(currCluster);
            if (currCluster > endOfCluster) { //If we wrap around
                break;
            }
            if (currCluster == endOfCluster) {
                currCluster = (currCluster+1)%table_size;
                currCluster = findClusterStart(currCluster);
            }  
            else {
                currCluster = endOfCluster;
            }   
        } while (true);
        this->delCount = 0;
}

void QuotientFilterGraveyard::redistributeTombstonesBetweenRuns() {
    if (opCount >= REBUILD_WINDOW_SIZE) {

        //Start by finding a cluster to process
        int initialStart = findClusterStart(0);
        int currCluster = initialStart;
        do {
            int endOfCluster = reorganizeCluster(currCluster);
            if (currCluster > endOfCluster) { //If we wrap around
                break;
            }
            if (currCluster == endOfCluster) {
                currCluster = (currCluster+1)%table_size;
                currCluster = findClusterStart(currCluster);
            }  
            else {
                currCluster = endOfCluster;
            }          
        } while (true);

        //Update the opcount and rebuild windows appropriately
        this->opCount = 0;
        float load_factor =  size/(double)table_size;
        float x = 1/(1-load_factor);
        this->REBUILD_WINDOW_SIZE = table_size/(4*x);
    }
}

void QuotientFilterGraveyard::redistributeTombstonesBetweenRunsInsert() {
    if (opCount >= REBUILD_WINDOW_SIZE) {
        //Start by finding a cluster to process
        int initialStart = findClusterStart(0);
        int currCluster = initialStart;
        do {
            int endOfCluster = reorganizeCluster2(currCluster);
            if (currCluster > endOfCluster) { //If we wrap around
                break;
            }
            if (currCluster == endOfCluster) {
                currCluster = (currCluster+1)%table_size;
                currCluster = findClusterStart(currCluster);
            } 
            else {
                currCluster = endOfCluster;
            }  
        } while (true);
        //Update the opcount and rebuild windows appropriately
        this->opCount = 0;
        float load_factor =  size/(double)table_size;
        float x = 1/(1-load_factor);
        this->REBUILD_WINDOW_SIZE = table_size/(4*x);
    }
}

void QuotientFilterGraveyard::redistributeTombstonesBetweenRunsEvenlyDistribute() {
    if (opCount >= REBUILD_WINDOW_SIZE) {
        //Add as many additional tombstones per graveyard hashing algorithm
        float load_factor =  size/(double)table_size;
        float x = 1/(1-load_factor);
        int numNewTombstones = table_size/(2*x);
        for (int i=0; i<numNewTombstones; i++) {
            int insertIndex = 2*i*x;
            if (!insertTombstone(insertIndex)){
                // std::cout << "Inserting tombstone at " << insertIndex << "\n"
                break;
            }
        }
        this->opCount = 0;
        load_factor =  size/(double)table_size;
        x = 1/(1-load_factor);
        this->REBUILD_WINDOW_SIZE = table_size/(4*x);
    }

}

void  QuotientFilterGraveyard::redistribute(){
    switch(this->redistributionPolicy) {
        case between_runs:
            redistributeTombstonesBetweenRuns();
            break;
        case between_runs_insert:
            redistributeTombstonesBetweenRunsInsert();
            break;
        case evenly_distribute:
            redistributeTombstonesBetweenRunsEvenlyDistribute();
            break;
        default:
            break;
    }
}
/**
 * 
 *         // for (int i=0; i < size; i++){
        //     std::cout<< "Element value at " << i << " is " << table[i].value << "\n";
        //     std::cout<< "Element shifted at " << i << " is " << table[i].is_shifted << "\n";
        // }

                // std::cout << "Table after\n";
        // for (int i=0; i < size; i++){
        //     std::cout<< "Element value at " << i << " is " << table[i].value << "\n";
        //     std::cout<< "Element shifted at " << i << " is " << table[i].is_shifted << "\n";
        // }
 * 
**/