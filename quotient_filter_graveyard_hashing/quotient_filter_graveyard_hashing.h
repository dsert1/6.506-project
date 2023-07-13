#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "../quotient_filter/quotient_filter_element.h"

enum RedistributionPolicy{
    no_redistribution, amortized_clean, between_runs, between_runs_insert, evenly_distribute
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
        void shiftTombstoneDown(int start, int predecessor, int successor, PairVal cleanUpInfo);
        void updateAdjacentTombstonesInsert(int newRun);
        int findRunStartForBucket(int target_bucket);
        void findRunStartForBucket(int target_bucket, PairVal* res);
        int findRunStartForBucket(int target_bucket, bool stop_at_tombstones);
        int findRunStartForBucket(int target_bucket, bool stop_at_tombstones, PairVal* res);
        int findEndOfCluster(int slot);
        int findEndOfRun(int startOfRun, int * predecessor, int * successor);
        void shiftElementsUp(int start, int bucketNumber);
        bool isEmptySlot(int slot, int bucket);

        int findClusterStart(int pos);
        int reorganizeCluster(int nextItem);
        int reorganizeCluster2(int nextItem);
        int startOfWrite(int start);
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
        int shiftClusterElementsDown(Res res, bool noleave_tombstones);
        bool insertTombstone(int pos);
        bool cleanAndSearch(PairVal cleanUpInfo, int bucketNumber, int remainderToFind);

        int findStartOfTombstonesInRun(int pos);
        Res findStartOfWriteAndCopy(int startOfCluster);
        Opt separateRunsByTombstones(int startOfCluster);

        QuotientFilterGraveyard(int q, int (*hashFunction)(int), RedistributionPolicy policy);
        ~QuotientFilterGraveyard();

        void insertElement(int value);
        void deleteElement(int value);
        bool query(int value);
        bool mayContain(int value);
        int findNextBucket(int currBucket, int endIndex=-1);
};