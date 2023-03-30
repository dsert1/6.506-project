#include "quotient_filter.h"
#include <cmath>

QuotientFilter::QuotientFilter(int q, int r, int (*hashFunction)(int)) { //Initialize a table of size 2^(q)
    QuotientFilter::size = 0;
    QuotientFilter::q = q;
    QuotientFilter::r = r;
    QuotientFilter::hashFunction = hashFunction;
}

void QuotientFilter::insertElement(int value) {
    FingerprintPair f = fingerprintQuotient(value)

    // Set occupied bit to 1

    // may_contain scan

}

void QuotientFilter::deleteElement(int value) {
    // Step 1: Figure out value finger print
    FingerprintPair f = fingerprintQuotient(value);
    if (table[f.fq].is_occupied) {
        // insert a tombstone
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
    // get value fingerprint
    FingerprintPair f = fingerprintQuotient(value);
    
    // check if the item is in the table
    if (table[f.fq].is_occupied) {
        // search for fingerprint remainder in table
        int start = f.fq;

        // look for value in the run
        while (start < size) {
            if (table[start].value == f.fr) {
                return true; // found value!
            }
            if (!table[start].is_continuation) {
                return false; // not found
            }
            start++;
        }
    }
    return false; // not found
}

bool QuotientFilter::mayContain(int value) {
    FingerprintPair f = fingerprintQuotient(value);

    // Check for item in table
    if (!table[f.fq].is_occupied) {
        return false;
    }

}

/* Helper function; 
*/
int findRunStartForBucket(int bucket) {

    // Backtrack to beginning of cluster
    int bucket = f.fq;
    while (table[b].is_shifted) {
        bucket--;
    }

    // Find the run for fq
    int bucket_slot = bucket;
    while (bucket != f.fq) {
        // Skip to the next run
        while (table[bucket_slot].is_continuation) {
            bucket_slot++;
        }
        // Find the next bucket
        do {
            bucket++;
        }
        while (!table[bucket].is_occupied);
    }
    
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