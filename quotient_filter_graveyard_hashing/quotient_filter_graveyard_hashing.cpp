#include "quotient_filter_graveyard_hashing.h"

QuotientFilterGraveyard::QuotientFilterGraveyard(int q, int r, int (*hashFunction)(int)) { //Initialize a table of size 2^(q)
    this->size = 0;
    this->q = q;
    this->r = r;
    this->hashFunction = hashFunction;

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
        *s = *(s) + 1;
    }
}

void QuotientFilterGraveyard::advanceToNextBucket(int * b) {
    while (!table[*b].is_occupied) {
        *b = *(b) + 1;
    }
}

bool QuotientFilterGraveyard::query(int value) {
}