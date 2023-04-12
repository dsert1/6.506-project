#include "quotient_filter_element.h"

QuotientFilterElement::QuotientFilterElement(long long int value, bool is_occupied, bool is_continuation, bool is_shifted) {
    this->value = value;
    this->is_occupied = is_occupied;
    this->is_continuation = is_continuation;
    this->is_shifted = is_shifted;
    this->isTombstone = false;
}