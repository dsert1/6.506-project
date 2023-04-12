struct FingerprintPair {
    int fq;
    int fr;
};

class QuotientFilterElement {
    public:
        int value;
        bool is_occupied;
        bool is_continuation;
        bool is_shifted;
        bool isTombstone;

        QuotientFilterElement(int value, bool is_occupied, bool is_continuation, bool is_shifted);
        
};
