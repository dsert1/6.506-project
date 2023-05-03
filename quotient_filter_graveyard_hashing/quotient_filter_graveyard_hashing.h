#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "../quotient_filter/quotient_filter_element.h"

class QuotientFilterGraveyard {
    private:
        float REDISTRIBUTE_UPPER_LIMIT = 0.60;
        float REDISTRIBUTE_LOWER_LIMIT = 0.15;
    public:
        QuotientFilterElement* table;
        int size;
        int q;
        int r;
        int table_size;
        int (*hashFunction)(int);
        RedistributionPolicy redistributionPolicy;
         
        FingerprintPair fingerprintQuotient(int value);
        void shiftTombstoneDown(int start, int predecessor, int successor);
        void advanceToNextRun(int * start);
        void advanceToNextBucket(int * start);
        void updateAdjacentTombstonesInsert(int newRun);
        int findRunStartForBucket(int target_bucket);
        int findEndOfCluster(int slot);
        void shiftElementsUp(int start);
        RunInfo findEndOfRunOrStartOfTombstones(int runStart, int bucketStart);
        bool runIsAllTombstones(int startOfRun);
        int findClusterStart(int pos);
        int reorganizeCluster(int nextItem);
        int startOfWrite(int start);
        int startOfCopy(int startOfMove);
        int correctStartOfCopyLoc(int startOfMove, int bucketOfToBeCopied);
        int findNextBucket(int start);
        QuotientFilterGraveyard(int q, int (*hashFunction)(int), RedistributionPolicy policy);
        ~QuotientFilterGraveyard();
        bool isEmptySlot(int slot, int bucket);
        void insertElement(int value);
        void deleteElement(int value);
        bool query(int value);
        bool mayContain(int value);
        long long int encodeValue(int predecessor, int successor);
        PredSucPair decodeValue(long long int value);
        // void shiftRunUp(int * bucketPos, int * runStart);
        void shiftRunUp(int startOfShift);
        void redistributeTombstones();
        void resetTombstoneSuccessors(int bucket);
};

enum RedistributionPolicy{
    no_redistribution, between_runs, between_runs_insert, evenly_distribute
}