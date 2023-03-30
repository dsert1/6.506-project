#include "quotient_filter_element.h"

class QuotientFilterGraveyard {
    private:
        QuotientFilterElement table[10];
        int size;
        int q;
        int r;
        int (*hashFunction)(int);
        FingerprintPair fingerprintQuotient(int value);
        void shiftElementsDown(int start);
    
    public:
        QuotientFilterGraveyard(int q, int r, int (*hashFunction)(int));
        void insertElement(int value);
        void deleteElement(int value);
        bool query(int value);
        bool mayContain(int value);
}