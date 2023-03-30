#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "quotient_filter_element.h"

// struct QuotientFilterElement {
//     int value;
//     bool is_occupied;
//     bool is_continuation;
//     bool is_shifted;
// };


class QuotientFilter {
    private:
        QuotientFilterElement table [10];
        int size;
        int q; // size of the quotient
        int r; // size of remainder. q+r = sizeof([insert value type])
        int (*hashFunction)(int);
        FingerprintPair fingerprintQuotient(int value);
        int findRunStartForBucket(int bucket)
        void shiftElementsDown(int start);
    
    public:
        QuotientFilter(int q, int r, int (*hashFunction)(int));
        void insertElement(int value);
        void deleteElement(int value);
        bool query(int value);
        bool mayContain(int value);
};