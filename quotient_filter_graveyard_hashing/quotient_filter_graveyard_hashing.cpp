#include "quotient_filter_graveyard_hashing.h"
#include <iostream>

/**
 * TODO: 
 * -> Finish the last function(D)
 * -> New flag for tombstone(D)
 * -> Correct setting of bits(D)
 * -> make sure that we don't swap into an earlier bucket ins hiftrun(D)
 * -> Correct how we get bucket of run we are copying(D)
 * -> Hash functions
 * -> Testing and fixing of any bugs(D)
 * -> Fix insert method to use the new boolean flag(D)
 * -> Have redistribute be the three different ideas: insert between runs, og and third variant
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
    this->REBUILD_WINDOW_SIZE = 0.4*this->table_size;
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

        bool valid_for_insertion;
        if (succ > pred) {
            valid_for_insertion = pred < f.fq && f.fq < succ;
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
    target_slot = findRunStartForBucket(f.fq);

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
    table[target_slot].is_shifted = (target_slot != f.fq); //Not equal!
    this->size += 1;
    this->opCount +=1;
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
        int successorBucket = f.fq;
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
                    std::cout << "MADE IT HERE FOR DELETING " << f.fq<<"\n";
                    resetTombstoneSuccessors(f.fq);
                }
            }
            std::cout << "HERE WITH is_occupied: "<< table[f.fq].is_occupied<<"\n";
            std::cout << "deletepointIndex: "<< deletePointIndex<< " successor Bucket" << successorBucket <<"\n";
            shiftTombstoneDown(deletePointIndex, f.fq, successorBucket);
            this->size--;
            // redistributeTombstones();
            std::cout << "HERE WITH is_occupied: "<< table[f.fq].is_occupied<<"\n";
        }
        this->opCount +=1;
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
        curr = (curr-1)%table_size;
    }

    std::cout << "TOMBSTONE FOUND AT " << startOfTombstones << "\n";

    //Step 2: If tombstone found, reset their values
    if (tombstoneFound) {
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
    std::cout << "prev pointer" << prevPointer << "\n";
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
    int target_slot = (newRun + 1) % this->table_size;
    while (table[target_slot].isTombstone) {
        PredSucPair oldPs = decodeValue(table[target_slot].value);
        table[target_slot].value = encodeValue(newRun, oldPs.successor);
        target_slot = (target_slot + 1) % this->table_size;
    }

    target_slot = (newRun - 1) % this->table_size;
    while (table[target_slot].isTombstone) {
        PredSucPair oldPs = decodeValue(table[target_slot].value);
        table[target_slot].value = encodeValue(oldPs.predecessor, newRun);
        target_slot = (target_slot - 1) % this->table_size;
    }
}

/* Helper function; assumes that the target bucket is occupied
*/
int QuotientFilterGraveyard::findRunStartForBucket(int target_bucket) {
    // Check for item in table
    if (!table[target_bucket].is_occupied) {
        return -1;
    }

    // std::cout << "Bucket before " << target_bucket << "\n";
    // Backtrack to beginning of cluster
    int bucket = target_bucket;
    while (table[bucket].is_shifted) {
        bucket = (bucket - 1) % table_size;
    }

    // std::cout << "Bucket after " << bucket << "\n";
    // Find the run for fq
    int run_start = bucket;
    while (bucket != target_bucket) {
        // std::cout << "Bucket: " <<bucket << " Run start: " << run_start << "\n";
        // Skip to the next run
        do {
            run_start = (run_start + 1) % table_size;
        }
        while (table[run_start].is_continuation);
        // Find the next bucket
        do {
            bucket = (bucket + 1) % table_size;
        }
        while (!table[bucket].is_occupied);
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
            std:: cout << "s: " << s << "\n";
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
    // std::bitset<32> y(hash);
    // std::cout << "HASH VALUE: " << y << "\n";
    FingerprintPair res;
    res.fq = hash >> this->r;
    // std::bitset<32> x(res.fq);
    // std::cout << "SHIFTED HASH: " << x << "\n";
    // std::cout << "HASH VALUE: " << hash << "\n";
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
    return start;
}

int QuotientFilterGraveyard::findClusterStart(int pos) {
    while(table[pos].is_shifted) { //walk backwards to start of cluster
        pos = (pos - 1 + table_size)%table_size;
    }
    return pos;
}

int QuotientFilterGraveyard::correctStartOfWrite(int start, int toBeCopiedBucket) {
    while (start < toBeCopiedBucket && table[start].is_shifted) { //make sure you are still in the cluster!
        start = (start + 1)%table_size;
    }
    return start;
}

int QuotientFilterGraveyard::reorganizeCluster(int startOfCluster){
    int startOfCopying = startOfCluster;
    int startOfWriting = startOfCluster;
    int startBucket;
    do {
        if (table[startOfWriting].isTombstone) { //If starts with tombstone
            startOfCopying = startOfCopy(startOfCopying);
            if (!table[startOfCopying].is_shifted) { //Cluster has  all tombstones so no things to copy over
                return  startOfCopying;
            } else {
                int tombstoneBefore = (startOfCopying-1+table_size)%table_size;
                PredSucPair predsuc  = decodeValue(table[tombstoneBefore].value);
                startBucket = predsuc.successor;
                startOfWriting = correctStartOfWrite(startOfWriting, startBucket);
            }
        } else {
            startOfWriting = startOfWrite(startOfWriting);
            if (!table[startOfWriting].is_shifted) {  //If there are no tombstones in the cluster, no rearrangement to be done
                return startOfWriting;
            } else {
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
                    int tombstoneBefore = (startOfCopying-1+table_size)%table_size;
                    PredSucPair predsuc  = decodeValue(table[tombstoneBefore].value);
                    startBucket = predsuc.successor;
                    startOfWriting = correctStartOfWrite(startOfWriting, startBucket);
                }
            }
        }
        //Keep copying until you get to end of run you are copying
        while(!table[startOfCopying].isTombstone && table[startOfCopying].is_shifted) {
            table[startOfWriting] =  table[startOfCopying];
            table[startOfCopying].isTombstone = true;
            startOfWriting = (startOfWriting + 1) % table_size;
            startOfCopying = (startOfCopying + 1) % table_size;
        }
        //If you stopped because you hit a tombstone, advance the writing pointer
        if (table[startOfCopying].isTombstone) {
            PredSucPair predsuc = decodeValue(table[startOfCopying].value);
            table[startOfWriting] = table[startOfCopying]; //Copy one tombstone there. 
            //Advance your writing p
            startOfWriting = (startOfWriting + 1) % table_size;
        } else { //If you stopeed because you hit a new cluster, leave the one tombstone and zero out the rest
            int startOfWriting = (startOfWriting + 1) % table_size;
            table[startOfWriting].isEndOfCluster = true;
            while (startOfWriting != startOfCopying) { //Empty out the tombstones after leaving just one
                table[startOfWriting].is_continuation = false;
                table[startOfWriting].isTombstone = false;
                table[startOfWriting].is_occupied = false;
                table[startOfWriting].is_shifted = false;
                table[startOfWriting].isEndOfCluster = false;
                startOfWriting = (startOfWriting + 1) % table_size;
            }
            // return startOfCopying;
        }
    } while (startOfCopying != startOfWriting);
    return startOfCopying;
}

int QuotientFilterGraveyard::findEndOfRun(int startOfRun) {
    if (table[startOfRun].is_continuation && table[startOfRun].is_shifted) {
        startOfRun = (startOfRun + 1) % table_size;
    }
    return startOfRun;
}

int QuotientFilterGraveyard::reorganizeCluster2(int startOfCluster, int endOfCluster){
    QuotientFilterElement replacedElement = QuotientFilterElement(-1,false,false,false,false);//Sentinel value
    int startOfShift = findEndOfRun(startOfCluster);
    do {

    } while(true);
}

void QuotientFilterGraveyard::redistributeTombstonesBetweenRuns() {
    if (opCount >= REBUILD_WINDOW_SIZE) {
        //Process each cluster as its own entity
        int initialStart = findClusterStart(0);
        int currCluster = initialStart; //find start of cluster 
        do {
            int endOfCluster = reorganizeCluster(currCluster);
            currCluster = endOfCluster;
        } while (currCluster != initialStart);

        //Update the opcount and rebuild windows appropriately
        this->opCount = 0;
        float load_factor =  size/(double)table_size;
        float x = 1/(1-load_factor);
        this->REBUILD_WINDOW_SIZE = size/(4*x);
    }
}

void QuotientFilterGraveyard::redistributeTombstonesBetweenRunsInsert() {
    float load_factor =  size/(double)table_size;
    if (opCount >= REBUILD_WINDOW_SIZE) {

        int initialStart = findClusterStart(0);
        int currStart = initialStart;
        do {
            int endOfCluster = reorganizeCluster2(currStart);
            currStart = endOfCluster;
        } while (currStart != initialStart);
        //Update the opcount and rebuild windows appropriately
        this->opCount = 0;
        float load_factor =  size/(double)table_size;
        float x = 1/(1-load_factor);
        this->REBUILD_WINDOW_SIZE = size/(4*x);
    }
}

//SUCCESSSOR = bucket of run immediately following successor
//PREDECESSOR = the bucket run I am in.