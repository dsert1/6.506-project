#include "quotient_filter_graveyard_hashing.h"
#include <iostream>
#include <deque>
#include<stack>

/**
 * TODO: 
 * -> Change how we update itemsTouched! Should be difference between start and end
 * -> Early termination for other redistribution policy
**/
QuotientFilterGraveyard::QuotientFilterGraveyard(int q, int (*hashFunction)(int), RedistributionPolicy policy=no_redistribution) { //Initialize a table of size 2^(q)
    this->size = 0;
    this->q = q;
    this->hashFunction = hashFunction;
    this->r = sizeof(int)*8 - q; //og : sizeof(int) - q
    this->table_size = (1 << q);
    this->table = (QuotientFilterElement*)calloc(sizeof(QuotientFilterElement), this->table_size);
    this->redistributionPolicy = policy;
    this->opCount=0;
    this->REBUILD_WINDOW_SIZE = 0.20*this->table_size; //figure out good numerical value for this based on quotient filter paper
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
    // std::cout << "Inserting initially: " << target_slot << "\n";
    if (table[target_slot].isTombstone) {

        PredSucPair ps = decodeValue(table[target_slot].value);
        int pred = ps.predecessor;
        int succ = ps.successor;
        // printf("insertion tombstone test: pred %d, bucket %d, succ %d\n", pred, f.fq, succ);

        bool valid_for_insertion;
        if (succ > pred) {
            valid_for_insertion = pred < f.fq && f.fq < succ;
        }
        else if (succ == pred) {
            // case of no successor
            valid_for_insertion = true;
        }
        else {
            // accounting for the case of wrapping around the end of the array
            valid_for_insertion = (pred < f.fq && f.fq < this->table_size) ||
                                  (0 < f.fq && f.fq < succ);
        }
        valid_for_insertion |= table[target_slot].isEndOfCluster;
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
    target_slot = findRunStartForBucket(f.fq, true);

    // Scan to end of run
    if (originally_occupied) {
        do {
            target_slot = (target_slot + 1) % table_size;
        } while (table[target_slot].is_continuation && !table[target_slot].isTombstone);
    }


    // shift following runs (Edit: only do this if element was not put in an empty slot)
    if (!isEmptySlot(target_slot, f.fq)) {
        shiftElementsUp(target_slot);
    }
    if (!originally_occupied) {
        updateAdjacentTombstonesInsert(target_slot);
    }

    // insert element
    table[target_slot].isTombstone = false;
    table[target_slot].value = f.fr;
    table[target_slot].is_continuation = originally_occupied;
    table[target_slot].is_shifted = (target_slot != f.fq);
    this->size += 1;

    //Perform redistribution if necessary
    this->opCount +=1;
    redistribute();
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

int max(int value1, int value2){
    if (value1 > value2) {
        return value1;
    }
    return value2;
}

int min(int value1, int value2){
    if (value1 < value2) {
        return value1;
    }
    return value2;
}

void QuotientFilterGraveyard::deleteElement(int value) {
    //Step 1: Figure out value finger print
    FingerprintPair f = fingerprintQuotient(value);
    //Step 2: If that location's fingerprint is occupied, insert tombstone and keep moving
    if (table[f.fq].is_occupied) {
        //Find predecessor value and position of element
        int s = findRunStartForBucket(f.fq);
        int startOfRun = s;
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
            int nextItem = (s + 1)%table_size;
            if (!table[nextItem].is_continuation|| table[nextItem].isTombstone) { //Set is_occupied accurately
                table[f.fq].is_occupied = false;
                //If run was shifted, it may affect the tombstones in the run where its actual bucket is located
                if (table[s].is_shifted) {
                    // std::cout << "MADE IT HERE FOR DELETING " << f.fq<<"\n";
                    resetTombstoneSuccessors(f.fq);
                }
            }
            // std::cout << "HERE WITH is_occupied: "<< table[f.fq].is_occupied<<"\n";
            // std::cout << "deletepointIndex: "<< deletePointIndex<< " successor Bucket" << successorBucket <<"\n";
            shiftTombstoneDown(deletePointIndex, f.fq, successorBucket);
            this->size--;
            // redistributeTombstones();
            // std::cout << "HERE WITH is_occupied: "<< table[f.fq].is_occupied<<"\n";
        }
        this->opCount +=1;
        //Perform redistribution if necessary
        redistribute();
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
        // std::cout << "TOMBSTONE FOUND AT " << startOfTombstones << "\n";
        PredSucPair predsuc = decodeValue(table[startOfTombstones].value);
        int currTombstone = startOfTombstones;
        while (table[currTombstone].is_continuation) {
            if (newSuccessorFound) {
                table[currTombstone].value = encodeValue(predsuc.predecessor, successor);
            } else { //If end of cluster, value unnecessary
              table[currTombstone].isEndOfCluster = true;  
            }
            currTombstone = (currTombstone + 1)%table_size;
        }
    }

}

void QuotientFilterGraveyard::shiftTombstoneDown(int afterTombstoneLocation, int tombstonePrededcessorBucket, int tombstoneSuccessorBucket) {
    int currPointer =  afterTombstoneLocation;
    int prevPointer = (currPointer - 1 + table_size)%table_size;
    // std::cout << "prev pointer" << prevPointer << "\n";
    int successorBucket = tombstoneSuccessorBucket;
    //Push tombstone to the end of the run  or to the start of  a stretch of tombstonese
    while (table[currPointer].is_continuation && !table[currPointer].isTombstone) {
        // std::cout << "IN HERE" << "\n";
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
    // std::cout << "Successor  Bucket At End: " << successorBucket << "\n";
    table[prevPointer].value = encodeValue(tombstonePrededcessorBucket, successorBucket);
    // std::cout << "ENCODED VALUE: " << table[prevPointer].value << "\n";
    table[prevPointer].isTombstone = true;
    table[prevPointer].isEndOfCluster = successorBucket == tombstonePrededcessorBucket;  //If successor was never updated, then we are at the end of the cluster
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
int QuotientFilterGraveyard::findRunStartForBucket(int target_bucket) {
    return this->findRunStartForBucket(target_bucket, false);
}

int QuotientFilterGraveyard::findRunStartForBucket(int target_bucket, bool stop_at_tombstones){
    // Check for item in table
    if (!table[target_bucket].is_occupied) {
        return -1;
    }

    // std::cout << "Bucket before " << target_bucket << "\n";
    // Backtrack to beginning of cluster
    int bucket = target_bucket;
    while (table[bucket].is_shifted) {
        bucket = (bucket - 1 + table_size) % table_size;
    }

    // std::cout << "Bucket after " << bucket << "\n";
    // Find the run for fq
    int run_start = bucket;
    int next_bucket = bucket;
    while (bucket != target_bucket) {
        // std::cout << "Bucket: " <<bucket << " Run start: " << run_start << "\n";
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
    // std::cout << "Bucket: " <<bucket << " Run start: " << run_start << "\n";
    return run_start;
}

/* Returns the first slot after the cluster containing the given slot
 * (i.e. guaranteed to return a different slot, assuming the table isn't full)
*/
int QuotientFilterGraveyard::findEndOfCluster(int slot) {
    int current_slot = slot;
    // std::cout << "START: " << slot << "\n";
    do {
        current_slot = (current_slot + 1) % table_size;
    }
    while (table[current_slot].is_shifted);

    return current_slot;
}

void QuotientFilterGraveyard::shiftElementsUp(int start) {
    int end = findEndOfCluster(start);
    // std::cout << "End of cluster: " << end << "\n";
    // Make sure the end of one cluster isn't the start of another
    while (table[end].is_occupied && !table[end].isTombstone) {
        end = findEndOfCluster(end);
    }

    // end should now point to an open position
    while (end != start) { //Corner case: End is an empty slot next to start.
        int target = (end - 1 + table_size) % table_size;
        table[end].isTombstone = table[target].isTombstone;
        table[end].value = table[target].value;
        table[end].is_continuation = table[target].is_continuation;
        table[end].is_shifted = true;
        end = target;
    }
    // while (!table[*b].is_occupied);
    // std::cout << "b after " << *b << "\n";
}


bool QuotientFilterGraveyard::query(int value) {
    FingerprintPair f = fingerprintQuotient(value);
    // std:: cout << "fq: " << f.fq << "\n";
    if (table[f.fq].is_occupied) {

        if (table[f.fq].value == f.fr) {
            return true;
        } else {
            int s = findRunStartForBucket(f.fq);
            // std:: cout << "s: " << s << "\n";
            //Once you locate the run containing item, look through to find it
            do {
                if (table[s].value == f.fr) {
                    return true;
                }
                s = (s+1)%table_size;
            }
            while (table[s].is_continuation && !table[s].isTombstone); //Stop when you see a tombstone
        }
    }
    return false;
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

int QuotientFilterGraveyard::startOfWrite(int start, int * itemsTouched) {
    while (!table[start].isTombstone && table[start].is_shifted) {
        start = (start + 1)%table_size;
        *(itemsTouched) = *(itemsTouched) + 1;//increment it each time you see an element
    }
    if (!table[start].isTombstone) { //You  have touched the element!
        *(itemsTouched) = *(itemsTouched) + 1;//increment it each time you see an element
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
        // std::cout << "Finding cluster with  start: " << start << "\n";
        //walk backwards to start of cluster if one exists
        while(table[start].is_shifted) {
            start = (start - 1 + table_size)%table_size;
        }
        // std::cout << "Ended with start: " << start << "\n";
        //At this point start, if it is truly an item, is pointing to the start of some cluster
        if (table[start].isTombstone || table[start].is_occupied){
            return start;
        } else {
            start = (start + 1) % table_size; //otherwise, search for the cluster elsewhere;
        }
    } while(start != pos);
    return start;
}

int QuotientFilterGraveyard::correctStartOfWrite(int start, int toBeCopiedBucket) {
    while (start < toBeCopiedBucket && table[start].is_shifted) { //make sure you are still in the cluster!
        start = (start + 1)%table_size;
    }
    return start;
}

Res QuotientFilterGraveyard::findStartOfWriteAndCopy(int startOfCluster, int * itemsTouched) {
    int startOfCopying = startOfCluster;
    int startOfWriting = startOfCluster;
    Res res;
    res.val4 = normal_case;
    int currBucket;
    if (table[startOfWriting].isTombstone) { //If starts with tombstone or empty spot
        // std::cout << "HERE TOMBSTONE GOT " << startOfWriting << "\n";
        startOfCopying = startOfCopy((startOfCopying + 1)%table_size);
        if (table[startOfCopying].is_shifted) { 
            PredSucPair predsuc  = decodeValue(table[startOfWriting].value);
            currBucket = predsuc.successor;
            startOfWriting = correctStartOfWrite((startOfWriting+1)%table_size, currBucket);
        }  else{ //Cluster has  all tombstones so no things to copy over
            res.val4 = all_tombstones;
        }
    } else {
        startOfWriting = startOfWrite((startOfWriting+1)%table_size, itemsTouched);
        // std::cout << "HERE NOT TOMBSTONE GOT " << startOfWriting << "\n";
        if (table[startOfWriting].is_shifted) {
            startOfCopying = startOfCopy(startOfWriting);
            if (!table[startOfCopying].is_shifted) { //If we don't have anything to move up in the cluster
                int start = (startOfWriting + 1) % table_size;
                while (start != startOfCopying) { //Empty out the tombstones after leaving just one
                    table[start].is_continuation = false;
                    table[start].isTombstone = false;
                    table[start].is_occupied = false;
                    table[start].is_shifted = false;
                    table[start].isEndOfCluster = false;
                    start = (start + 1) % table_size;
                }
                res.val4 = nothing_to_push;
            } else {
                int tombstoneBefore = (startOfCopying-table_size)%table_size;
                PredSucPair predsuc  = decodeValue(table[tombstoneBefore].value);
                currBucket = predsuc.successor;
                startOfWriting = correctStartOfWrite(startOfWriting, currBucket);
            }
        } else{ //If there are no tombstones in the cluster, no rearrangement to be done
            // std::cout << "NO TOMSTONES\n"; 
            res.val4 = no_tombstones;
        }
    }
    // std::cout << "final start of copying: " << startOfCopying << "final start of writing: " << startOfWriting << "final current Bucket at the end: " << currBucket << "\n";
    res.val1 = startOfCopying;
    res.val2 = startOfWriting;
    res.val3 = currBucket;
    return res;
}

/**
 * Reorganizes the a cluster by moving down elements in runs down if there are available tombstones
 * */
int QuotientFilterGraveyard::reorganizeCluster(int startOfCluster, int * itemsTouched){
    // std::cout << "Reorganizing cluster starting at " << startOfCluster << "\n";
    Res res = findStartOfWriteAndCopy(startOfCluster, itemsTouched);
    switch(res.val4) {
        case all_tombstones:
            // std::cout << "ALL TOMBSTONES when reorganizing "<< startOfCluster <<"\n";
        case no_tombstones:
            // std::cout << "ALL TOMBSTONES when reorganizing "<< startOfCluster <<"\n";
        case nothing_to_push:
            // std::cout << "NOTHING TO DO when reorganizing "<< startOfCluster <<"\n";
            return res.val1;
        default:
            return shiftClusterElementsDown(res, itemsTouched);
    }
}

int QuotientFilterGraveyard::shiftClusterElementsDown(Res res, int * itemsTouched) {
    int startOfCopying = res.val1;
    int startOfWriting = res.val2;
    int currBucket = res.val3;
    if (startOfCopying != startOfWriting) {
        // std::cout << "when copying happens start Of Write: " << startOfWriting << " when copying happens final start of Copy: " << startOfCopying << "\n";
        int startBucket = currBucket;
        do { //Copy all runs, separated by a tombstone
            //Set start bucket appropriately
            // std::cout << "Start of writing: " << startOfWriting << " Start Of Reading: " << startOfCopying <<  " startBucket: " << currBucket << "\n";
            do {
                //Copy over element, maintaining is_occupied bit
                bool initialOccupiedBit = table[startOfWriting].is_occupied;
                table[startOfWriting] = table[startOfCopying];
                table[startOfWriting].is_occupied = initialOccupiedBit;
                table[startOfWriting].is_shifted = !(startOfWriting == startBucket);
                //Zero out copied element
                table[startOfCopying].isTombstone = true;
                *(itemsTouched) = *(itemsTouched) + 1;
                //Advance pointers and get bucket of next run if not already got
                startOfWriting = (startOfWriting + 1) % table_size;
                startOfCopying = (startOfCopying + 1) % table_size;
                if (table[startOfWriting].is_occupied && currBucket == startBucket){
                    // std::cout << "HERE AT index: " << startOfWriting << "\n";
                    currBucket = startOfWriting;
                }
            }
            while(!table[startOfCopying].isTombstone && table[startOfCopying].is_continuation);  //Copy a single run

            // std::cout << "Run ended at: " << startOfCopying  << "currBucket: " << currBucket << "\n";
            //Add a tombstone after the run to break up cluster.
            table[startOfWriting].isTombstone = true;
            table[startOfWriting].isEndOfCluster = true;
            startOfWriting = (startOfWriting + 1) % table_size;
            //Check to see if we still have room to write
            if (startOfCopying == startOfWriting){
                break;
            }
            //get bucket of next run if not already got
            if (table[startOfWriting].is_occupied && currBucket == startBucket){
                // std::cout << "HERE AT index: " << startOfWriting << "\n";
                currBucket = startOfWriting;
            }
            // Check to see if you need to advance startOfCopying and by extension advance startOfWriting accordingly
            if (table[startOfCopying].isTombstone) {
                PredSucPair predsuc  = decodeValue(table[startOfCopying].value);
                startOfWriting = correctStartOfWrite(startOfWriting, currBucket);
                startOfCopying = startOfCopy(startOfCopying);
            }
            startBucket = currBucket;
        } while (table[startOfCopying].is_shifted); //do this process while we are still within the cluster

        //Empty out the tombstones after leaving just one
        if (startOfWriting != startOfCopying) {
            table[startOfWriting].isEndOfCluster = true;
            int startOfWriting = (startOfWriting + 1) % table_size;
            while (startOfWriting != startOfCopying) { 
                table[startOfWriting].is_continuation = false;
                table[startOfWriting].isTombstone = false;
                table[startOfWriting].is_occupied = false;
                table[startOfWriting].is_shifted = false;
                table[startOfWriting].isEndOfCluster = false;
                startOfWriting = (startOfWriting + 1) % table_size;
            }
        }
    }
    return startOfCopying;
}

int QuotientFilterGraveyard::findEndOfRun(int startOfRun, int * itemsTouched) { //Function assumes we have no tombstones in the run
    Pair res;
    // std::cout << "Finding end of run starting at: " << startOfRun << "\n";
    do {
        startOfRun = (startOfRun + 1) % table_size;
        if (!table[startOfRun].isTombstone) {
            *(itemsTouched) = *(itemsTouched)+1;
        }
    } while (table[startOfRun].is_continuation && table[startOfRun].is_shifted);
    return startOfRun;
}

Opt QuotientFilterGraveyard::separateRunsByTombstones(int startOfCluster, int * itemsTouched) {
    Opt opt;
    std::deque<QuotientFilterElement> backedUpElements;
    std::stack<int> lastDisplaced;
    // std::cout << "IN HERE\n";
    //Find end of run and push onto queue
    int nextSlot = findEndOfRun(startOfCluster, itemsTouched); //Really the next thing after the run ends
    backedUpElements.push_back(table[nextSlot]);
    lastDisplaced.push(nextSlot);

    //Calculate the bucket of run
    int prevBucket = startOfCluster;
    int currBucket = findNextBucket(prevBucket);
    do {
        //Place tombstone at the next slot and push index to stack
        table[nextSlot].isTombstone = true;
        table[nextSlot].value = encodeValue(prevBucket, currBucket); //the tombstone is for the preceeding run
        table[nextSlot].isEndOfCluster = false; //we are merging clusters!
        table[nextSlot].is_continuation = true;
        table[nextSlot].is_shifted = true;
        // std::cout << "NEW RUN\n";
        //Shift items from the queue into the nextSlot until you hit the end of a run
        do {
            //pop element to place down
            QuotientFilterElement currElement = backedUpElements.front();
            // std::cout << "NEXT SLOT" << nextSlot << "\n";
            // std::cout << "CURR ELEMENT" << currElement.value << "\n";
            backedUpElements.pop_front();

            //Element may be empty slot
            if (currElement.isTombstone|| (!currElement.is_occupied && !currElement.is_shifted)){
                opt.val2 = true;
                opt.val1 = nextSlot;
                return opt;
            } else { //element is an actual value
                *(itemsTouched) = *(itemsTouched) + 1; //Count element
                //Create room to place element
                nextSlot++;
                //If you wrap around, undo all placements up until necessary and return
                if (nextSlot == startOfCluster){
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
                    opt.val1 = nextSlot;
                    return opt;
                }
                backedUpElements.push_back(table[nextSlot]);
                lastDisplaced.push(nextSlot);    lastDisplaced.push(nextSlot);
                //Place element making sure to maintain is_occupied bit
                bool initialIsOccupied = table[nextSlot].is_occupied;
                table[nextSlot] = currElement;
                table[nextSlot].is_occupied = initialIsOccupied; //is_occupied bit is independent of run, so do not change!
                table[nextSlot].is_shifted = true; //clusters that were once separate are now conjoined so merge them
            }
        }while (backedUpElements.front().is_continuation && !backedUpElements.front().isTombstone);//Keep putting elements down until we put down a run
        //Make allowance for tombstone at the end of the run
        nextSlot++;
        backedUpElements.push_back(table[nextSlot]);
        lastDisplaced.push(nextSlot);
        prevBucket = currBucket;
        currBucket = findNextBucket(currBucket);
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

int QuotientFilterGraveyard::reorganizeCluster2(int startOfCluster, int * itemsTouched){
    //Start by trying to reorganize as in policy1
    Res res = findStartOfWriteAndCopy(startOfCluster, itemsTouched);
    switch(res.val4) {
        case all_tombstones:
        case nothing_to_push:
            return res.val1;
        case no_tombstones:
            return separateRunsByTombstones(startOfCluster, itemsTouched).val1;
        default:
            int initialRes = shiftClusterElementsDown(res, itemsTouched);
            if (table[initialRes].is_shifted){ //If we are still within the cluster, then we continue processing
                return separateRunsByTombstones(initialRes, itemsTouched).val1;
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
    //Step 1: Check if position is either a tombstone or empty before doing any work
    if (table[pos].isTombstone) {
        // std::cout << "POSITION IS TOMBSTONE\n";
        return true;
    } else if (!table[pos].is_occupied && !table[pos].is_shifted) {
        // std::cout << "POSITION IS EMPTY\n";
        table[pos].isTombstone = true;
        table[pos].is_continuation = true;
        table[pos].isEndOfCluster = true;
        return true;
    } else { //Step 2: Otherwise, we must go to the end of the run
        // std::cout << "POSITION OCCUPIED\n";
        int endOfRun = findStartOfTombstonesInRun(pos); //next position after just ended
        // std::cout << "end of run starting at " << pos << ": " << endOfRun << "\n";
        //Slot may be empty, a tombstone or an element
        if (!table[endOfRun].is_occupied && !table[endOfRun].is_shifted) { //Case 1: empty
            table[endOfRun].isTombstone = true;
            table[endOfRun].is_continuation = true;
            table[endOfRun].isEndOfCluster = true;
            table[endOfRun].is_shifted = true;
            return true;
        } else if (table[endOfRun].isTombstone) { //Case 2: tombstone
            // std::cout << "POSITION IS A TOMBSTONE\n";
            return true;
        } else { //begin process of shifting elements down after inserting tombstone
            //Put replaced element in Queue
            std::deque<QuotientFilterElement> replacedElements;
            std::stack<int> lastDisplaced;
            QuotientFilterElement replacedElement = table[endOfRun];
            lastDisplaced.push(endOfRun);
            replacedElements.push_back(replacedElement);

            //Put tombstone at end of run
            table[endOfRun].isTombstone = true;
            table[endOfRun].is_continuation = true;
            table[endOfRun].isEndOfCluster = true;
            table[endOfRun].is_shifted = true;

            int start = endOfRun+1;
            QuotientFilterElement temp = table[start];
            //until the replaced element is empty, keep shifting element down
            while (!(!replacedElement.is_occupied && !replacedElement.is_shifted)){

                if (start == pos) { //If we wrap around because we have no more room for tombstones, undo everything
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
                int prevStart = (start - 1 + table_size)%table_size;
                table[start].is_shifted = !(prevStart == start);
                lastDisplaced.push(start);
                replacedElements.push_back(temp);
                start = (start + 1) % table_size;
            }
            
            return true;
        }
    }
}

void QuotientFilterGraveyard::redistributeTombstonesBetweenRuns() {
    if (opCount >= REBUILD_WINDOW_SIZE) {
        //Start by finding a cluster to process
        int initialStart = findClusterStart(0);
        int currCluster = initialStart; 
        int itemsTouched = 1;  //count one for the start!
        do {
            int endOfCluster = reorganizeCluster(currCluster, &itemsTouched);
            if (currCluster == endOfCluster) {
                currCluster++;
                currCluster = findClusterStart(currCluster);
            } else {
                currCluster = endOfCluster;
            }
            // std::cout << "ITEMS TOUCHED" << itemsTouched << "\n";
        } while (itemsTouched < size);

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
        int itemsTouched = 1; //count one for the start!
        do {
            int endOfCluster = reorganizeCluster2(currCluster, &itemsTouched);
            if (currCluster == endOfCluster) {
                currCluster++;
                currCluster = findClusterStart(currCluster);
            } else {
                currCluster = endOfCluster;
            }
        } while (itemsTouched < size);
        //Update the opcount and rebuild windows appropriately
        this->opCount = 0;
        float load_factor =  size/(double)table_size;
        float x = 1/(1-load_factor);
        this->REBUILD_WINDOW_SIZE = table_size/(4*x);
    }
}

void QuotientFilterGraveyard::redistributeTombstonesBetweenRunsEvenlyDistribute() {
    if (opCount >= REBUILD_WINDOW_SIZE) {
        // std::cout << "Redistributing\n";
        //Start by cleaning up table
        redistributeTombstonesBetweenRuns();
        // std::cout << "Finished cleaning up table. Printing out ......\n";

        // std::cout << "Finished printing out table. Inserting tombstones artificially......\n";
        //Add as many additional tombstones per graveyard hashing algorithm
        float load_factor =  size/(double)table_size;
        float x = 1/(1-load_factor);
        int numNewTombstones = table_size/(2*x);
        for (int i=0; i<numNewTombstones; i++) {
            int insertIndex = 2*i*x;
            // std::cout << "Inserting tombstone at " << insertIndex << "\n";
            if (!insertTombstone(insertIndex)){
                break;
            }
        }
        //Update the opcount and rebuild windows appropriately
        this->opCount = 0;
        this->REBUILD_WINDOW_SIZE = table_size/(4*x);
    }

    // for (int i=0; i<8; i++) {
    //     std::cout<< "PRINTING OUT INFO AT: " << i << "\n";
    //     if (table[i].isTombstone) {
    //         PredSucPair res = decodeValue(table[i].value);
    //         std::cout << "PREDECESSOR: " <<res.predecessor << "\n";
    //         std::cout << "SUCCESSOR: " <<res.successor << "\n";
    //     } else {
    //         std::cout << table[i].value << "\n";
    //     }
    //     std::cout << "IS OCCUPIED: " <<table[i].is_occupied << "\n";
    //     std::cout << "IS SHIFTED: " <<table[i].is_shifted << "\n";
    //     std::cout << "IS CONTINUATION: " <<table[i].is_continuation << "\n";
    // }

    // for (int i=13; i<16; i++) {
    //     std::cout<< "PRINTING OUT INFO AT: " << i << "\n";
    //     if (table[i].isTombstone) {
    //         PredSucPair res = decodeValue(table[i].value);
    //         std::cout << "PREDECESSOR: " <<res.predecessor << "\n";
    //         std::cout << "SUCCESSOR: " <<res.successor << "\n";
    //     } else {
    //         std::cout << table[i].value << "\n";
    //     }
    //     std::cout << "IS OCCUPIED: " <<table[i].is_occupied << "\n";
    //     std::cout << "IS SHIFTED: " <<table[i].is_shifted << "\n";
    //     std::cout << "IS CONTINUATION: " <<table[i].is_continuation << "\n";
    // }
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

//SUCCESSSOR = bucket of run immediately following successor
//PREDECESSOR = the bucket run I am in.
/**
 * Triple QuotientFilterGraveyard::findStartOfWriteAndCopy(int startOfCluster, int * itemsTouched) {
    int startOfCopying = startOfCluster;
    int startOfWriting = startOfCluster;
    Triple res;
    int currBucket;
    // do {
    //Initialize your startOfWrite and copy appropriately
    std::cout << "initial start Of Write: " << startOfWriting << " initial start of Copy: " << startOfCopying << "\n";
    if (table[startOfWriting].isTombstone) { //If starts with tombstone
        startOfCopying = startOfCopy((startOfCopying + 1)%table_size);
        if (!table[startOfCopying].is_shifted) { //Cluster has  all tombstones so no things to copy over
            return  startOfCopying;
        } else {
            PredSucPair predsuc  = decodeValue(table[startOfWriting].value);
            currBucket = predsuc.successor;
            startOfWriting = correctStartOfWrite((startOfWriting+1)%table_size, currBucket);
        }
    } else {
        startOfWriting = startOfWrite((startOfWriting+1)%table_size, itemsTouched);
        if (!table[startOfWriting].is_shifted) {  //If there are no tombstones in the cluster, no rearrangement to be done
            return startOfWriting;
        }else {
            startOfCopying = startOfCopy(startOfWriting);
            if (!table[startOfCopying].is_shifted) { //If we don't have anything to move up in the cluster
                int start = (startOfWriting + 1) % table_size;
                while (start != startOfCopying) { //Empty out the tombstones after leaving just one
                    table[start].is_continuation = false;
                    table[start].isTombstone = false;
                    table[start].is_occupied = false;
                    table[start].is_shifted = false;
                    table[start].isEndOfCluster = false;
                    start = (start + 1) % table_size;
                }
                return startOfCopying;  //Return next cluster start
            } else {
                int tombstoneBefore = (startOfCopying-table_size)%table_size;
                PredSucPair predsuc  = decodeValue(table[tombstoneBefore].value);
                currBucket = predsuc.successor;
                startOfWriting = correctStartOfWrite(startOfWriting, currBucket);
            }
        }
    }
    res.val1 = startOfCopying;
    res.val2 = startOfWriting;
    res.val3 = currBucket;
}
        // //Case 1: It is an empty spot
        // if (!nextSlotElement.is_occupied && !nextSlotElement.is_shifted) { //If the next slot is empty, fill in items from the queue until 
        //     std::cout << "replaced element is a empty space\n";
        //     table[startOfNextRun].isTombstone  = true;
        //     table[startOfNextRun].isEndOfCluster = true;
        //     table[startOfNextRun].is_continuation = true;
        //     table[startOfNextRun].is_shifted = true;
        //     //Break up the cluster and make it a new one. On the next iteration, it will be dealt with.
        //     int nextItem = (startOfNextRun + 1)%table_size;
        //     // table[nextItem].is_shifted = false;
        //     // table[nextItem].is_continuation = false;
        //     opt.val2 = true;
        //     opt.val1 = nextItem;
        //     return opt;
        // } else  if (nextSlotElement.isTombstone){  //Case 2: It is a tombstone. We shift things down appropriately
        //     std::cout << "replaced element is a tombstone\n";
        //     //Begin 
        //     int nextItem = (startOfNextRun + 1)%table_size;
        //     Res res = findStartOfWriteAndCopy(nextItem, itemsTouched);

        //     //Shift down runs as much as you can
        //     int nextClusterStart = shiftClusterElementsDown(res, itemsTouched);
        //     opt.val1 = nextClusterStart;
        //     if (table[nextClusterStart].is_shifted) { //If we didn't finish processing the cluster
        //         opt.val2 = false;
        //     } else {
        //         opt.val1 = true;
        //     }
        //     return opt;
        // } else { //Case 3: it is an actual element
        //     //Otherwise, we are either merging two separate clusters or spreading out a cluster
        //     std::cout << "replaced element is NOT a tombstone and is NOT empty\n";
        //     table[startOfNextRun].isTombstone = true;
        //     table[startOfNextRun].value = encodeValue(prevBucket, currBucket); //the tombstone is for the preceeding run
        //     table[startOfNextRun].isEndOfCluster = false; //we are merging clusters!
        //     table[startOfNextRun].is_continuation = true;
        //     table[startOfNextRun].is_shifted = true;

        //     //Shift down in run until
        //     int start = startOfNextRun;
        //     QuotientFilterElement temp  = table[start];
        //     do {
        //         //repeatedly shift things around till you get to the end of the run
        //         start = (start + 1) %  table_size;
        //         temp  = table[start];
        //         std::cout << "START INDEX: " << start << "\n";
        //          //If we wrap all the way around, put temp back where you last placed a tombstone if it is not itself a tombstone
        //         if (start == startOfCluster) {
        //             if (!temp.isTombstone) {
        //                 int initialOccupiedBit = table[start].is_occupied;
        //                 table[startOfNextRun] = temp;
        //                 table[startOfNextRun].is_occupied = initialOccupiedBit;
        //             }
        //             opt.val1 = true;
        //             opt.val2 = start;
        //             return opt;
        //         }
        //         backedUpElements.push(temp); //push element on to queue
        //         backedUpElements.pop(); //feel free to now pop the replacedElement
        //         table[start] = nextSlotElement;
        //         table[start].is_occupied = temp.is_occupied; //is_occupied bit is independent of run, so do not change!
        //         table[start].is_shifted = true; //clusters that were once separate are now conjoined so merge them
        //         nextSlotElement = backedUpElements.front();
        //     }while (temp.is_continuation && !temp.isTombstone); //stop in the run if you see a tombstone, proceed to do as before
        //     //Put a tombstone at the end of this run

        //     prevBucket = currBucket;
        //     currBucket = findNextBucket(currBucket);
        //     startOfNextRun = start;
 * 
 **/