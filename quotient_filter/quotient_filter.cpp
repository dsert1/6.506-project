#include "quotient_filter.h"
#include <cmath>

QuotientFilter::QuotientFilter(int q, int r, int (*hashFunction)(int)) { //Initialize a table of size 2^(q)
    this->size = 0;
    this->q = q;
    this->r = r;
    this->hashFunction = hashFunction;

    this->table_size = (1 << q);
    this->table = calloc(sizeof(QuotientFilterElement), this->table_size);
}

QuotientFilter::~QuotientFilter() {
    free(this->table);
}

void QuotientFilter::insertElement(int value) {
    // make sure table isn't full
    if (this->size == this->table_size) {
        return;
    }

    FingerprintPair f = fingerprintQuotient(value);
    bool originally_occupied = table[f.fq].is_occupied;

    // Set occupied bit to 1
    table[f.fq].is_occupied = 1;

    // Find the beginning of the run
    int target_slot = findRunStartForBucket(f.fq);

    // Scan to end of run
    if (originally_occupied) {
        do {
            target_slot = (target_slot + 1) % table_size;
        } while (table[target_slot].is_continuation);
    }
    
    // shift following runs
    shiftElementsUp(target_slot);

    // insert element
    table[target_slot].value = value;
    table[target_slot].is_continuation = originally_occupied;
    table[target_slot].is_shifted = (target_slot == f.fq);
    this->size += 1;
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
    /* This is actually essentially the query function;
        if anything we might be able to rewrite that to use
        findRunStartForBucket (lets you avoid some of the value comparisons) */

    return query(value);
}

/* Helper function; assumes that the target bucket is occupied
*/
int findRunStartForBucket(int target_bucket) {
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

FingerprintPair QuotientFilter::fingerprintQuotient(int value) {
    int hash = this->hashFunction(value);
    FingerprintPair res;
    res.fq = hash >> this->r;
    res.fr = hash % (1 << this->r);
    return res;
}

// Returns the first slot after the cluster containing the given slot
int QuotientFilter::findEndOfCluster(int slot) {
    int current_slot = slot;

    while (table[current_slot].is_shifted) {
        current_slot = (current_slot + 1) % table_size;
    }

    return current_slot;
}

void QuotientFilter::shiftElementsDown(int start) {
    int currPointer = start;
    while (currPointer < size && table[currPointer].is_occupied) {
        table[currPointer-1] = table[currPointer];
        currPointer++;
    }
}

void QuotientFilter::shiftElementsUp(int start) {
    int end = findEndOfCluster(start);
    // Make sure the end of one cluster isn't the start of another
    while (table[end].is_occupied) {
        end = findEndOfCluster(end);
    }

    // end should now point to an open position
    while (end != start) {
        int target = (end - 1) % table_size;
        table[end].value = table[target].value;
        table[end].is_continuation = table[target].is_continuation;
        table[end].is_shifted = true;
        end = target;
    }    
}
// Potential optimization: only move the elements at the beginnings of runs to the end?