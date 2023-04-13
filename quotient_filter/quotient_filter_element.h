struct FingerprintPair {
    int fq;
    int fr;
};

struct PredSucPair {
    int predecessor;
    int successor;
};

class QuotientFilterElement {
    public:
        long long int value;
        bool is_occupied;
        bool is_continuation;
        bool is_shifted;
        bool isTombstone;

        QuotientFilterElement(long long int value, bool is_occupied, bool is_continuation, bool is_shifted);
        
};
