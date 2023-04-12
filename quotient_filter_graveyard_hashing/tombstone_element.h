#include "../quotient_filter/quotient_filter_element.h"


class TombstoneElement : public QuotientFilterElement {
    private:
        int predecessorFq;
        int successorFq;
    
    public:
        TombstoneElement(int predecessorFq, int successorFq, int is_continuation, int is_shifted);


};