#include "quotient_filter_graveyard_hashing.h"

QuotientFilterGraveyard::QuotientFilterGraveyard(int q, int (*hashFunction)(int)) { //Initialize a table of size 2^(q)
    this->size = 0;
    this->q = q;
    this->hashFunction = hashFunction;

    this->r = sizeof(int) - q;
    this->table_size = (1 << q);
    this->table = calloc(sizeof(QuotientFilterElement), this->table_size);
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
    while (!table[*b].is_occupied) {
        *b = (*(b) + 1) % this_table_size;
    }
}

bool QuotientFilterGraveyard::query(int value) {
    // figure out value fingerprint
    FingerprintPair f = fingerprintQuotient(value);
    int quotient = f.fq;
    int remainder = f.fr;

    // compute hash values for the three possible positions of the element
    int h1 = hashFunction(value);

    // check if any of the three positions contains the element
    int s = h1 % this->table_size;
    if (table[s].is_occupied && table[s].value == value) {
        return true;
    }

    // check if the element is in a graveyard slot
    int b = h1 % this->table_size;
    while (table[b].is_continuation) {
        if (fingerprintQuotient(value).fq == table[b].value) {
            return true;
    }
    b = (b + 1) % this->table_size;
}

    // element isnt present in the filter
    return false;
}

// // get value fingerprint
//     FingerprintPair f = fingerprintQuotient(value);
    
//     // check if the item is in the table
//     if (table[f.fq].is_occupied) {
//         // search for fingerprint remainder in table
//         int start = f.fq;

//         // look for value in the run
//         while (start < size) {
//             if (table[start].value == f.fr) {
//                 return true; // found value!
//             }
//             if (!table[start].is_continuation) {
//                 return false; // not found
//             }
//             start++;
//         }
//     }
//     return false; // not found