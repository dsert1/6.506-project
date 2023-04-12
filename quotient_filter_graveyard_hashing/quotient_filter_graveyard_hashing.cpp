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
        // insert element
        // update adjacent tombstones

    // otherwise:
        // Find the beginning of run
        // Scan to end of run if existing
        // Update adjacent tombstones if new
        // Shift following runs (can shift into tombstones)
        // insert element

    // increment size
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