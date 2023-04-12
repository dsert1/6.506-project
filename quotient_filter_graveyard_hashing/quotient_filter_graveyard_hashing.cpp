#include "quotient_filter_graveyard_hashing.h"
#include <iostream>

QuotientFilterGraveyard::QuotientFilterGraveyard(int q, int (*hashFunction)(int)) { //Initialize a table of size 2^(q)
    this->size = 0;
    this->q = q;
    this->hashFunction = hashFunction;

    this->r = 8 - q; //og : sizeof(int) - q
    this->table_size = (1 << q);
    this->table = (QuotientFilterElement*)calloc(sizeof(QuotientFilterElement), this->table_size);
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
    bool tombstone_check_placeholder = false;
    if (tombstone_check_placeholder) {
        int pred = target.predecessorFq;
        int succ = target.successorFq;
        bool valid_for_insertion;
        if (succ > pred) {
            valid_for_insertion = pred < f.fq && f.fq < succ;
        } else {
            // accounting for the case of wrapping around the end of the array
            valid_for_insertion = (pred < f.fq && f.fq < this->table_size) ||
                                  (0 < f.fq && f.fq < succ);
        }

        if (valid_for_insertion) {
            // TODO: Mark as no longer being a tombstone
            table[target_slot].value = f.fr;
            table[target_slot].is_continuation = 0;
            table[target_slot].is_shifted = 0;
            this->size += 1;

            updateAdjacentTombstonesInsert(f.fq);
            return;
        }
    }

    // no valid tombstone:

    // Find the beginning of the run
    target_slot = findRunStartForBucket(f.fq);

    // Scan to end of run
    if (originally_occupied) {
        do {
            target_slot = (target_slot + 1) % table_size;
            tombstone_check_placeholder = false;
        } while (table[target_slot].is_continuation && !tombstone_check_placeholder);
    }

    // shift following runs
    shiftElementsUp(target_slot);
    if (!originally_occupied) {
        updateAdjacentTombstonesInsert(target_slot);
    }

    // insert element
    // TODO: Mark as no longer being a tombstone
    table[target_slot].value = f.fr;
    table[target_slot].is_continuation = originally_occupied;
    table[target_slot].is_shifted = (target_slot == f.fq);
    this->size += 1;
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

        //Check to make sure there aren't other members of the run before setting to false!!
        int nextItem = (f.fq + 1)%table_size;
        if (!table[nextItem].is_continuation) {
            table[f.fq].is_occupied = false;
        }
        int startOfCluster = f.fq;
        while (table[startOfCluster].is_shifted) {
            startOfCluster = ((startOfCluster-1)%table_size + table_size)%table_size;
        }

        //Using bits in the Quotient filter element, locate the run of fr
        int s = startOfCluster;
        int b = startOfCluster;
        while (b != f.fq && b<table_size){
            advanceToNextRun(&s);
            advanceToNextBucket(&b);
        }

        //Place a tombstone at the end of the run within which it exists
        int startOfRun = s;
        int deletePointIndex;
        int predecessorBucket = s;
        bool found = false;
        do {
            if (table[startOfRun].value == f.fr) {
                deletePointIndex = (startOfRun+1)%table_size;
                found = true;
            }
            if (table[startOfRun].is_occupied) {
                predecessorBucket = startOfRun;
            }
            startOfRun = (startOfRun+1)%table_size;
        }
        while (table[startOfRun].is_continuation);

        //Check if the element is found before inserting tombstone at the end of run
        if (found) {
            shiftTombstoneDown(deletePointIndex, predecessorBucket);
            this->size--;
            redistributeTombstones();
        }
    }
}

void QuotientFilterGraveyard::shiftTombstoneDown(int afterTombstoneLocation, int tombstonePrededcessorBucket) {
    int currPointer =  afterTombstoneLocation;
    int prevPointer = ((currPointer-1)%table_size + table_size)%table_size;
    int predecessorBucket = tombstonePrededcessorBucket;
    std::cout << "Predecessor  Bucket: " << predecessorBucket << "\n";
    while (table[currPointer].is_continuation) { //Push tombstone to the end of the run
        table[prevPointer].value = table[currPointer].value;
        if (table[prevPointer].is_occupied) {
            predecessorBucket = prevPointer;
        }
        if (table[currPointer].isTombstone) {
            break;
        }
        currPointer = (currPointer + 1)%table_size;
        prevPointer = ((currPointer-1)%table_size + table_size)%table_size;
    }
    std::cout << "Predecessor  Bucket At End: " << predecessorBucket << "\n";
    table[prevPointer].value = encodeValue(predecessorBucket, prevPointer);
    table[prevPointer].isTombstone = true;
    table[prevPointer].is_continuation = true;
}

long long int QuotientFilterGraveyard::encodeValue(int predecessor, int successor) {
    //Shift preedecessor to top 32 bits and successor to lower 32 bits 
    if (predecessor > successor) {
        predecessor = 0;
    }
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

void QuotientFilterGraveyard::advanceToNextRun(int * s) {
    do {
        *s = (*(s) + 1)%table_size;
    }
    while (table[*s].is_continuation);
}

void QuotientFilterGraveyard::advanceToNextBucket(int * b) {
    std::cout << "b before " << *b << "\n";
    do {
        *b = (*(b) + 1)%table_size;
    }
    while (!table[*b].is_occupied);
    // std::cout << "b after " << *b << "\n";
}

// Updates the predecessors and successors of tombstones adjacent to a newly created run.
void QuotientFilterGraveyard::updateAdjacentTombstonesInsert(int newRun) {
    int target_slot = (newRun + 1) % this->table_size;
    bool tombstone_check_placeholder = false;
    while (tombstone_check_placeholder) {
        table[target_slot].predecessorFq = newRun;
        target_slot = (target_slot + 1) % this->table_size;
    }

    target_slot = (newRun - 1) % this->table_size;
    tombstone_check_placeholder = false;
    while (tombstone_check_placeholder) {
        table[target_slot].successorFq = newRun;
        target_slot = (target_slot - 1) % this->table_size;
    }
}

/* Helper function; assumes that the target bucket is occupied
*/
int QuotientFilter::findRunStartForBucket(int target_bucket) {
    // Check for item in table
    if (!table[target_bucket].is_occupied) {
        return -1;
    }

    // Backtrack to beginning of cluster
    int bucket = target_bucket;
    while (table[bucket].is_shifted) {
        bucket = (bucket - 1) % table_size;
    }

    // Find the run for fq
    int run_start = bucket;
    while (bucket != target_bucket) {
        // Skip to the next run
        while (table[run_start].is_continuation) {
            run_start = (run_start + 1) % table_size;
        }
        // Find the next bucket
        do {
            bucket = (bucket + 1) % table_size;
        }
        while (!table[bucket].is_occupied);
    }

    return run_start;
}

/* Returns the first slot after the cluster containing the given slot
 * (i.e. guaranteed to return a different slot, assuming the table isn't full)
*/
int QuotientFilter::findEndOfCluster(int slot) {
    int current_slot = slot;

    do {
        current_slot = (current_slot + 1) % table_size;
    }
    while (table[current_slot].is_shifted);

    return current_slot;
}

void QuotientFilter::shiftElementsUp(int start) {
    int end = findEndOfCluster(start);
    // Make sure the end of one cluster isn't the start of another
    bool tombstone_check_placeholder = false;
    while (table[end].is_occupied && !tombstone_check_placeholder) {
        end = findEndOfCluster(end);
    }

    // end should now point to an open position
    while (end != start) {
        int target = (end - 1) % table_size;
        // TODO: Mark as no longer being a tombstone
        table[end].value = table[target].value;
        table[end].is_continuation = table[target].is_continuation;
        table[end].is_shifted = true;
        end = target;
    }
}

bool QuotientFilterGraveyard::query(int value) {
    FingerprintPair f = fingerprintQuotient(value);
    std:: cout << "fq: " << f.fq << " fr: " << f.fr << "\n";

    if (table[f.fq].is_occupied) {
        if (table[f.fq].value == f.fr) {
            return true;
        } else {
            int startOfCluster = f.fq;
            while (table[startOfCluster].is_shifted) {
                startOfCluster = ((startOfCluster-1)%table_size + table_size)%table_size;
            }

            std:: cout << "Start of cluster: " << startOfCluster << "\n";
            int s = startOfCluster;
            int b = startOfCluster;
            while (b != f.fq){
                advanceToNextRun(&s);
                advanceToNextBucket(&b);
                // std:: cout << "s: " << s << " b: " << b << "\n";
            }
            // std:: cout << "s: " << s << " b: " << b << "\n";
            //Once you locate the run containing item, look through to find it
            do {
                if (table[s].value == f.fr) {
                    return true;
                }
                s = (s+1)%table_size;
            }
            while (table[s].is_continuation && !table[s].isTombstone);
        }
    }
    return false;
}

FingerprintPair QuotientFilterGraveyard::fingerprintQuotient(int value) {
    int hash = this->hashFunction(value);
    FingerprintPair res;
    res.fq = hash >> this->r;
    res.fr = hash % (1 << this->r);
    return res;
}

void QuotientFilterGraveyard::shiftRunUp(int * bucketPos, int * runStart) {
    int fillInPointer = *bucketPos;
    do {
        table[fillInPointer] = table[*runStart];
        fillInPointer++;
        *(runStart) = *((runStart) + 1)%table_size;
    }
    while (table[*runStart].is_continuation && !table[*runStart].isTombstone);

    //Allow 1 tombStone buffer because we know at least one thing needs to be moved
    *(runStart) = *(runStart) + 1;
    table[*runStart].isTombstone = true;
    table[*runStart].is_occupied = true;
}


void QuotientFilterGraveyard::redistributeTombstones() {
    //Start looping through from the beginning.
    float load_factor =  size/(double)table_size;
    if (load_factor > REDISTRIBUTE_UPPER_LIMIT ||  load_factor <= REDISTRIBUTE_LOWER_LIMIT) {
        int b = 0;
        int s = 0;
        if (table[b].isTombstone) {

            //Truncate the run to the back of the list if it is a tombstone
            do {
                table[b].isTombstone = false;
                table[b].is_continuation = false;
                b++;
            }
            while (table[b].isTombstone  && b<table_size);
        }
        //Move elements around and truncate tombstones
        b = 0;
        while (true) {
            advanceToNextBucket(&b);
            advanceToNextRun(&s);
            if (b != s) { //Shift the run there if it is not occupied
                shiftRunUp(&b,&s);
            }
            if (s==0) {
                break;
            }
        }
    }

}