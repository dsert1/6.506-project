#include "quotient_filter.h"
#include <cmath>
#include <iostream>

QuotientFilter::QuotientFilter(int q, int (*hashFunction)(int)) { //Initialize a table of size 2^(q)
    this->size = 0;
    this->q = q;
    this->hashFunction = hashFunction;

    this->r = sizeof(long long int)*8 - q; //multiply by 8 since sizeof gives no. of bits og: sizeof(long long int)*8 - q
    // std::cout << r;
    this->table_size = (1 << q);
    this->table = (QuotientFilterElement*)calloc(sizeof(QuotientFilterElement), this->table_size);
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
    // int target_slot=0;

    // Scan to end of run
    if (originally_occupied) {
        do {
            target_slot = (target_slot + 1) % table_size;
        } while (table[target_slot].is_continuation);
    }
    
    // shift following runs
    shiftElementsUp(target_slot);

    // insert element
    table[target_slot].value = f.fr;
    table[target_slot].is_continuation = originally_occupied;
    table[target_slot].is_shifted = (target_slot == f.fq);
    this->size += 1;
}

void QuotientFilter::deleteElement(int value) {
    // Step 1: Figure out value finger print
    FingerprintPair f = fingerprintQuotient(value);
    // std::cout << "fq: " << f.fq << " fr: " << f.fr  << "\n";
    //Step 2: If that location's finger print is occupied, check to see if you can find fr
    if (table[f.fq].is_occupied) {
        std:: cout << "fq: " << f.fq << " fr: " << f.fr << "\n";
        //Check to make sure there aren't other members of the run before setting to false!!
        int nextItem = (f.fq + 1)%table_size;
        if (!table[nextItem].is_continuation) {
            table[f.fq].is_occupied = false;
        }

        // Try to find the beginning of the cluster by walking backward
        int startOfCluster = f.fq;
        while (table[startOfCluster].is_shifted) {
            startOfCluster = ((startOfCluster-1)%table_size + table_size)%table_size;
        }

        //Using bits in the Quotient filter element, try to narrow down a range to look for fr
        int s = startOfCluster;
        int b = startOfCluster;
        while (b != f.fq && b<table_size){
            advanceToNextRun(&s);
            advanceToNextBucket(&b);
        }

        std::cout << "startOfRun: " << s << " bucket: " << b  << "\n";
        //Now we look for fr in that run and delete if found. We shift all elements in the run down
        int startOfRun = s;
        int deletePointIndex;
        int deletePointBucket;
        bool found = false;
        do {
            if (table[startOfRun].value == f.fr) {
                deletePointIndex = (startOfRun+1)%table_size;
                found = true;
            }
            startOfRun = (startOfRun+1)%table_size;
        }
        while (table[startOfRun].is_continuation);

        std::cout << "startIndexOfShifting: " << deletePointIndex << " beforeStart: " << -1%table_size << " startBucketOfShifting: " << b  << "\n";
        //Check if element found before shifting down
        if (found) {
            std:: cout << "HERE" << "\n";
            deletePointBucket = b;
            shiftElementsDown(deletePointIndex, deletePointBucket);
            this->size--;
        }

        // std::cout  << "DONE"<<"\n";

    }
}

void QuotientFilter::advanceToNextRun(int * s) {
    do {
        *s = (*(s) + 1)%table_size;
    }
    while (table[*s].is_continuation);
}

void QuotientFilter::advanceToNextBucket(int * b) {
    std::cout << "b before " << *b << "\n";
    do {
        *b = (*(b) + 1)%table_size;
    }
    while (!table[*b].is_occupied);
    // std::cout << "b after " << *b << "\n";
}

bool QuotientFilter::query(int value) {
    // get value fingerprint
    FingerprintPair f = fingerprintQuotient(value);
    std:: cout << "fq: " << f.fq << " fr: " << f.fr << "\n";
    // check if the item is in the table
    if (table[f.fq].is_occupied) {
        //If item in bucket already, return true
        if (table[f.fq].value == f.fr) {
            return true;
        } else {  //Otherwise, find the correct run
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
            while (table[s].is_continuation);
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

FingerprintPair QuotientFilter::fingerprintQuotient(int value) {
    int hash = this->hashFunction(value);
    FingerprintPair res;
    res.fq = hash >> this->r;
    res.fr = hash % (1 << this->r);
    return res;
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

void QuotientFilter::shiftElementsDown(int startIndex, int startBucket) {
    int currPointer = startIndex;
    int currBucket = startBucket;
    int prevPointer = ((currPointer-1)%table_size + table_size)%table_size;
    while (table[currPointer].is_shifted) {
        std::cout << "currPointer: " << currPointer << " currBucket: " << currBucket << " prevPointer: " << prevPointer <<"\n";
        //If within the same run, shift value over only
        table[prevPointer].value = table[currPointer].value;
        table[prevPointer].is_shifted = !(currBucket == prevPointer);
        table[prevPointer].is_occupied = currBucket == prevPointer;
        //If encounter different run, check that we haven't shifted an element to its correct bucket
        if (!table[currPointer].is_continuation) {
            std::cout << "HERE 2" << "\n";
            currBucket++;
            table[prevPointer].is_continuation = false;
        } else {
            table[prevPointer].is_continuation = !(currBucket==prevPointer);
        }
        currPointer = (currPointer + 1)%table_size;
        prevPointer = ((currPointer-1)%table_size + table_size)%table_size;
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