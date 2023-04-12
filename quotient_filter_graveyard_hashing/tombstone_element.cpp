#include "tombstone_element.h"

TombstoneElement::TombstoneElement(int predecessorFq, int successorFq, int is_continuation, int is_shifted) {
    this->predecessorFq = predecessorFq;
    this->successorFq = successorFq;
    this->is_continuation = is_continuation;
    this->is_shifted = is_shifted;
    this->is_occupied = false;
    this->isTombstone = true;
}