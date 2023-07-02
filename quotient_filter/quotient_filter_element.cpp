#include "quotient_filter_element.h"

QuotientFilterElement::QuotientFilterElement(unsigned long long int value, bool is_occupied, bool is_continuation, bool is_shifted, bool is_end_of_cluster) {
    this->value = value;
    this->is_occupied = is_occupied;
    this->is_continuation = is_continuation;
    this->is_shifted = is_shifted;
    this->isTombstone = false;
}