#include <stdint.h>
struct FingerprintPair {
    uint32_t fq;
    uint32_t fr;
};

struct PredSucPair {
    uint32_t predecessor;
    uint32_t successor;
};


struct RunInfo {
    uint32_t predecessor;
    uint32_t successor;
    bool isEndOfCluster;
    int endOfRunOrStartOfTombstones;
};

class QuotientFilterElement {
    public:
        unsigned long long int value;
        bool is_occupied;
        bool is_continuation;
        bool is_shifted;
        bool isTombstone;
        bool isEndOfCluster;
        QuotientFilterElement(unsigned long long int value, bool is_occupied, bool is_continuation, bool is_shifted, bool is_end_of_cluster);
        
};
