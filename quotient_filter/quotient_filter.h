#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "quotient_filter_element.h"


class QuotientFilter {
    private:
        QuotientFilterElement* table;
        int size; // number of elements contained
        int q; // size of the quotient
        int r; // size of remainder. q+r = sizeof([insert value type])
        int table_size; // size of the table
        int (*hashFunction)(int);

        FingerprintPair fingerprintQuotient(int value);
        int findRunStartForBucket(int bucket);
        int findEndOfCluster(int slot);
        void shiftElementsDown(int start);
        void shiftElementsUp(int start);
        void advanceToNextBucket(int * start);
        void advanceToNextRun(int * start);
    
    public:
        QuotientFilter(int q, int r, int (*hashFunction)(int));
        ~QuotientFilter();

        void insertElement(int value);
        void deleteElement(int value);
        bool query(int value);
        bool mayContain(int value);
};