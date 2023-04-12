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
}

void QuotientFilterGraveyard::advanceToNextRun(int * s) {
    while (table[*s].is_continuation) {
        *s = (*(s) + 1) % this->table_size;
    }
}

void QuotientFilterGraveyard::advanceToNextBucket(int * b) {

}

bool QuotientFilterGraveyard::query(int value) {
}