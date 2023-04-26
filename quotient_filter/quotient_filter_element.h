struct FingerprintPair {
    uint32_t fq;
    uint32_t fr;
};

struct PredSucPair {
    uint32_t predecessor;
    uint32_t successor;
};

class QuotientFilterElement {
    public:
        unsigned long long int value;
        bool is_occupied;
        bool is_continuation;
        bool is_shifted;
        bool isTombstone;

        QuotientFilterElement(unsigned long long int value, bool is_occupied, bool is_continuation, bool is_shifted);
        
};
