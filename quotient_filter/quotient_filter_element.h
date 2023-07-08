#include <stdint.h>
#pragma once
struct FingerprintPair {
    uint32_t fq;
    uint32_t fr;
};

#pragma once
struct PredSucPair {
    uint32_t predecessor;
    uint32_t successor;
};

#pragma once
struct RunInfo {
    uint32_t predecessor;
    uint32_t successor;
    bool isEndOfCluster;
    int endOfRunOrStartOfTombstones;
};

struct PairVal {
    int runStart;
    bool cleanUpNeeded;
    int tombstoneStart;
};

#pragma once
class QuotientFilterElement {
    public:
        unsigned long long int value;
        bool is_occupied;
        bool is_continuation;
        bool is_shifted;
        bool isTombstone;
        QuotientFilterElement(unsigned long long int value, bool is_occupied, bool is_continuation, bool is_shifted, bool is_end_of_cluster);
        
};
