#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "quotient_filter_element.h"


class QuotientFilter {
    private:
        FingerprintPair fingerprintQuotient(int value);
        int findRunStartForBucket(int bucket);
        int findFirstEmptySlot(int slot);
        void shiftElementsDown(int startIndex, int startBucket);
        void shiftElementsUp(int start);
        void advanceToNextBucket(int * start);
        void advanceToNextRun(int * start);
    
    public:
        int size; // number of elements contained
        QuotientFilterElement* table;
        int q; // size of the quotient
        int r; // size of remainder. q+r = sizeof([insert value type])
        int table_size; // size of the table
        int (*hashFunction)(int);
        QuotientFilter(int q, int (*hashFunction)(int));
        ~QuotientFilter();
        void insertElement(int value);
        void deleteElement(int value);
        bool query(int value);
        bool mayContain(int value);
};