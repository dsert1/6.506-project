#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "../quotient_filter/quotient_filter_element.h"

class QuotientFilterGraveyard {
    private:
        QuotientFilterElement* table;
        int size;
        int q;
        int r;
        int table_size;
        int (*hashFunction)(int);

        FingerprintPair fingerprintQuotient(int value);
        void shiftElementsDown(int start);
        void advanceToNextRun(int * start);
        void advanceToNextBucket(int * start);
    
    public:
        QuotientFilterGraveyard(int q, int (*hashFunction)(int));
        ~QuotientFilterGraveyard();

        void insertElement(int value);
        void deleteElement(int value);
        bool query(int value);
        bool mayContain(int value);
};