#include "quotient_filter.h"
#include <iostream>

int identity(int x) {
    return x;
}
std::ostream& operator<<(std::ostream &s, const QuotientFilter &point) {
    for (int i=0; i< point.size; i++) {
        s << (*(point.table+i)).value;
    }
    return s;
    // return s << "(" << point.x << ", " << point.y << ")";
}

int main () {
    QuotientFilter g = QuotientFilter(2,2, &identity);
    g .insertElement(5);
    std::cout << g;
}