#include "quotient_filter.h"
// #include "quotient_filter_element.h"
#include <iostream>

int identity(int x) {
    return x;
}
// std::ostream& operator<<(std::ostream &s, const QuotientFilter &point) {
//     for (int i=0; i< point.size; i++) {
//         s << (*(point.table+i)).value;
//     }
//     return s;
//     // return s << "(" << point.x << ", " << point.y << ")";
// }


void testDeleteOnly(){
    QuotientFilter g = QuotientFilter(4,&identity); 
    QuotientFilterElement* start = g.table;
    int elements[10] = {10,20,5,7,18,33};
    *start = QuotientFilterElement(10,true,false,false);
    start++;
    *start = QuotientFilterElement(5,true,true,true);
    start++;
    *start = QuotientFilterElement(7,true,true,true);
    start++;
    *start = QuotientFilterElement(2, false,false,true);
    start++;
    *start = QuotientFilterElement(4, false,true,true);
    start++;
    *start = QuotientFilterElement(1,false,false,true);
    g.size = 6;
    g.deleteElement(10);
    g.deleteElement(33);
    g.deleteElement(20);
    QuotientFilterElement* newStart = g.table;
    int count = 0;
    while (count < 3){
        count++;
        std::cout << "fr: " <<(*newStart).value << "\n";
        std::cout << "is_occupied: " <<(*newStart).is_occupied << "\n";
        std::cout << "is_continuation: " <<(*newStart).is_continuation << "\n";
        std::cout << "is_shifted: " <<(*newStart).is_shifted << "\n";
        newStart++;
    }

}

void testQuery()  {
    QuotientFilter g = QuotientFilter(4,&identity); 
    QuotientFilterElement* start = g.table;
    int elements[10] = {10,20,5,7,18,33};
    *start = QuotientFilterElement(10,true,false,false);
    start++;
    *start = QuotientFilterElement(5,true,true,true);
    start++;
    *start = QuotientFilterElement(7,true,true,true);
    start++;
    *start = QuotientFilterElement(2, false,false,true);
    start++;
    *start = QuotientFilterElement(4, false,true,true);
    start++;
    *start = QuotientFilterElement(1,false,false,true);
    std::cout << g.query(33) << "\n";
    std::cout << g.query(5) << "\n";
}


int main () {
    std::cout << "Started Delete" << "\n";
    testDeleteOnly();
    std::cout << "Ended Delete" << "\n";

}