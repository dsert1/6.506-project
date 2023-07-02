#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "../quotient_filter/quotient_filter_element.h"

enum RedistributionPolicy{
    no_redistribution, between_runs, between_runs_insert, evenly_distribute
};

enum ReorgCase{
    all_tombstones = 1,
    nothing_to_push = 2,
    no_tombstones = 3,
    normal_case= 4
};

struct Pair {
    int val1;
    int val2;
};

struct Opt {
    int val1;
    bool val2;
};

struct Res {
    int val1;
    int val2;
    int val3;
    ReorgCase val4;
};

class QuotientFilterGraveyard {
    private:
        int REBUILD_WINDOW_SIZE;
        int CLEANUP_WINDOW_SIZE;
    public:
        QuotientFilterElement* table;
        int size;
        int q;
        int r;
        int table_size;
        int (*hashFunction)(int);
        RedistributionPolicy redistributionPolicy;
        int opCount;
        int delCount;

         
        FingerprintPair fingerprintQuotient(int value);
        void shiftTombstoneDown(int start, int predecessor, int successor);
        void updateAdjacentTombstonesInsert(int newRun);
        int findRunStartForBucket(int target_bucket);
        int findRunStartForBucket(int target_bucket, bool stop_at_tombstones);
        int findEndOfCluster(int slot);
        int findEndOfRun(int startOfRun, int * itemsTouched);
        void shiftElementsUp(int start);
        bool isEmptySlot(int slot, int bucket);

        int findClusterStart(int pos);
        int reorganizeCluster(int nextItem, int * itemsTouched);
        int reorganizeCluster2(int nextItem, int * itemsTouched);
        int startOfWrite(int start, int * elementCount);
        int startOfCopy(int startOfMove);
        int correctStartOfWrite(int startOfMove, int bucketOfToBeCopied);

        long long int encodeValue(int predecessor, int successor);
        PredSucPair decodeValue(long long int value);
        void cleanUpTombstones();
        int cleanUpHelper(int start);
        void redistributeTombstonesBetweenRuns();
        void redistributeTombstonesBetweenRunsInsert();
        void redistributeTombstonesBetweenRunsEvenlyDistribute();
        void resetTombstoneSuccessors(int bucket);
        void redistribute();
        int shiftClusterElementsDown(Res res, int * itemsTouched, bool noleave_tombstones);
        bool insertTombstone(int pos);

        int findStartOfTombstonesInRun(int pos);
        Res findStartOfWriteAndCopy(int startOfCluster, int * itemsTouched);
        Opt separateRunsByTombstones(int startOfCluster, int * itemsTouched);

        QuotientFilterGraveyard(int q, int (*hashFunction)(int), RedistributionPolicy policy);
        ~QuotientFilterGraveyard();

        void insertElement(int value);
        void deleteElement(int value);
        bool query(int value);
        bool mayContain(int value);
        int findNextBucket(int currBucket, int endIndex=-1);
};