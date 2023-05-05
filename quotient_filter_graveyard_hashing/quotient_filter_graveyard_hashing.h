#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "../quotient_filter/quotient_filter_element.h"

class QuotientFilterGraveyard {
    private:
        int REBUILD_WINDOW_SIZE;
    public:
        QuotientFilterElement* table;
        int size;
        int q;
        int r;
        int table_size;
        int (*hashFunction)(int);
        RedistributionPolicy redistributionPolicy;
        int opCount;
         
        FingerprintPair fingerprintQuotient(int value);
        void shiftTombstoneDown(int start, int predecessor, int successor);
        void updateAdjacentTombstonesInsert(int newRun);
        int findRunStartForBucket(int target_bucket);
        int findEndOfCluster(int slot);
        int findEndOfRun(int slot);
        void shiftElementsUp(int start);
        bool isEmptySlot(int slot, int bucket);

        int findClusterStart(int pos);
        int reorganizeCluster(int nextItem);
        int reorganizeCluster2(int nextItem, int endOfCluster);
        int startOfWrite(int start);
        int startOfCopy(int startOfMove);
        int correctStartOfWrite(int startOfMove, int bucketOfToBeCopied);

        long long int encodeValue(int predecessor, int successor);
        PredSucPair decodeValue(long long int value);
        void redistributeTombstonesBetweenRuns();
        void redistributeTombstonesBetweenRunsInsert();
        void redistributeTombstonesBetweenRunsEvenlyDistribute();
        void resetTombstoneSuccessors(int bucket);

        QuotientFilterGraveyard(int q, int (*hashFunction)(int), RedistributionPolicy policy);
        ~QuotientFilterGraveyard();

        void insertElement(int value);
        void deleteElement(int value);
        bool query(int value);
        bool mayContain(int value);
};

enum RedistributionPolicy{
    no_redistribution, between_runs, between_runs_insert, evenly_distribute
};