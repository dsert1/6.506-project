#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

struct QuotientFilterElement {
    int value;
    bool is_occupied;
    bool is_continuation;
    bool is_shifted;
};

struct FingerprintPair {
    int fr; // fingerprint remainder
    int fq; // fingerprint quotient
};

class QuotientFilter {
    private:
        QuotientFilterElement table [10];
        int size;
        int q; // size of the quotient
        int r; // size of remainder. q+r = sizeof([insert value type])
        int (*hashFunction)(int);
        FingerprintPair fingerprintQuotient(int value);
        void shiftElementsDown(int start);
    
    public:
        QuotientFilter(int q, int r, int (*hashFunction)(int));
        void insertElement(int value);
        void deleteElement(int value);
        bool query(int value);
        bool mayContain(int value);
};