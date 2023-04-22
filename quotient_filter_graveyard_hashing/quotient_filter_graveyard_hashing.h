#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "../quotient_filter/quotient_filter_element.h"

class QuotientFilterGraveyard {
    private:
        float REDISTRIBUTE_UPPER_LIMIT = 0.6;
        float REDISTRIBUTE_LOWER_LIMIT = 0.15;
    public:
        QuotientFilterElement* table;
        int size;
        int q;
        int r;
        int table_size;
        int (*hashFunction)(int);

        FingerprintPair fingerprintQuotient(int value);
        void shiftTombstoneDown(int start, int predecessor);
        void advanceToNextRun(int * start);
        void advanceToNextBucket(int * start);
    
        QuotientFilterGraveyard(int q, int (*hashFunction)(int));
        ~QuotientFilterGraveyard();

        void insertElement(int value);
        void deleteElement(int value);
        bool query(int value);
        bool mayContain(int value);
        long long int encodeValue(int predecessor, int successor);
        PredSucPair decodeValue(long long int value);
        void shiftRunUp(int * bucketPos, int * runStart);
        void redistributeTombstones();
};