#include "quotient_filter_graveyard_hashing.h"

QuotientFilterGraveyard::QuotientFilterGraveyard(int q, int (*hashFunction)(int)) { //Initialize a table of size 2^(q)
    this->size = 0;
    this->q = q;
    this->hashFunction = hashFunction;

    this->r = sizeof(int) - q;
    this->table_size = (1 << q);
    this->table = (QuotientFilterElement*)calloc(sizeof(QuotientFilterElement), this->table_size);
}

QuotientFilterGraveyard::~QuotientFilterGraveyard() {
    free(this->table);
}

void QuotientFilterGraveyard::insertElement(int value) {

}

void QuotientFilterGraveyard::deleteElement(int value) {
    //Step 1: Figure out value finger print
    FingerprintPair f = fingerprintQuotient(value);

    //Step 2: If that location's ffingerprint is occupied, insert tombstone and keep moving
    if (table[f.fq].is_occupied) {

        table[f.fq].is_occupied = false;
        int startOfCluster = f.fq;
        while (table[startOfCluster].is_shifted) {
            startOfCluster--;
        }

        //Using bits in the Quotient filter element, locate the run of fr
        int s = startOfCluster;
        int b = startOfCluster;
        int prev_b = startOfCluster; //need to remember predecessor bucket
        while (b != f.fq && b<table_size){
            advanceToNextRun(&s);
            prev_b = b;
            advanceToNextBucket(&b);
        }

        //Place a tombstone at the end of the run within which it exists
        int startOfRun = s;
        int deletePointIndex;
        int deletePointBucket;
        do {
            if (table[startOfRun].value == f.fr) {
                deletePointIndex = startOfRun+1;
            }
            startOfRun++;
        }
        while (table[startOfRun].is_continuation);

        //Check if the element is found before inserting tombstone at the end of run
        if (table[deletePointIndex-1].value == f.fr) {
            deletePointBucket = b;
            shiftTombstoneDown(deletePointIndex, prev_b);
            this->size--;
        }
    }
}

void QuotientFilterGraveyard::shiftTombstoneDown(int afterTombstoneLocation, int predecessorOfTombstone) {
    int currPointer =  afterTombstoneLocation;
    while (currPointer < table_size && table[currPointer].is_continuation) {
        table[currPointer-1].value = table[currPointer].value;
        if (table[currPointer].isTombstone) {
            break;
        }
    }
    table[currPointer-1].value = computeValue(predecessorOfTombstone, predecessorOfTombstone+1);
}

long long int computeValue(int predecessor, int successor) {

}

void QuotientFilterGraveyard::advanceToNextRun(int * s) {
    do {
        *s = *(s) + 1;
    }
    while (*s < table_size && table[*s].is_continuation);
}

void QuotientFilterGraveyard::advanceToNextBucket(int * b) {
    do {
        *b = *(b) + 1;
    }
    while (*b < table_size && !table[*b].is_occupied);
}


bool QuotientFilterGraveyard::query(int value) {
}