#include "quotient_filter.h"
#include <cmath>

QuotientFilter::QuotientFilter(int q, int r, int (*hashFunction)(int)) { //Initialize a table of size 2^(q)
    QuotientFilter::size = 0;
    QuotientFilter::q = q;
    QuotientFilter::r = r;
    QuotientFilter::hashFunction = hashFunction;
}

void QuotientFilter::insertElement(int value) {

}

void QuotientFilter::deleteElement(int value) {
    // Step 1: Figure out value finger print
    FingerprintPair f = fingerprintQuotient(value);
    if (table[f.fq].is_occupied) {
        // insert a tombstone
        table[f.fq].is_occupied = false;
        int start = f.fq;
        while (start < size) {
            if (table[start].value == f.fr) {
                shiftElementsDown(start+1);
                break;
            }
            start++;
        }
    }

}

bool QuotientFilter::query(int value) {

}

bool QuotientFilter::mayContain(int value) {

}

FingerprintPair QuotientFilter::fingerprintQuotient(int value) {
    int hash = QuotientFilter::hashFunction(value);
    FingerprintPair res;
    res.fq = hash >> QuotientFilter::r;
    res.fr = hash % static_cast<int>(std::pow(2, QuotientFilter::r));
    return res;
}

void QuotientFilter::shiftElementsDown(int start) {
    int currPointer = start;
    while (currPointer < size && table[currPointer].is_occupied) {
        table[currPointer-1] = table[currPointer];
        currPointer++;
    }
}