#include "quotient_filter.h"
// #include "quotient_filter_element.h"
// #include <boost/python.hpp>
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

void testDeleteOnly2(){
    QuotientFilter g = QuotientFilter(3,&identity); 
    QuotientFilterElement* start = g.table;
    int elements[10] = {43,58,100,229,225};
    *start = QuotientFilterElement(5,false,true,true); //0: 229
    start++;
    *start = QuotientFilterElement(26,true,false,false); //1: 58
    start++;
    *start = QuotientFilterElement(11,false,true,true); //2: 43
    start++;
    *start = QuotientFilterElement(4, true,false,false);//3: 100
    start++;
    start++;
    start++;
    start++;
    *start = QuotientFilterElement(1, true,false,false); //7:225
    // g.table_size = 5;
    g.deleteElement(225);
    g.deleteElement(100);
    // g.deleteElement(20);
    QuotientFilterElement* newStart = g.table;
    int count = 0;
    while (count < g.table_size){
        std::cout << "index: " << count << "\n";
        std::cout << "fr: " <<(*newStart).value << "\n";
        std::cout << "is_occupied: " <<(*newStart).is_occupied << "\n";
        std::cout << "is_continuation: " <<(*newStart).is_continuation << "\n";
        std::cout << "is_shifted: " <<(*newStart).is_shifted << "\n";
        std::cout << "--------" << "\n";
        newStart++;
        count++;
    }

}

void testQuery()  {
    QuotientFilter g = QuotientFilter(3,&identity); 
    QuotientFilterElement* start = g.table;
    int elements[10] = {43,58,100,229,225};
    *start = QuotientFilterElement(5,false,true,true); //0: 229
    start++;
    *start = QuotientFilterElement(26,true,false,false); //1: 58
    start++;
    *start = QuotientFilterElement(11,false,true,true); //2: 43
    start++;
    *start = QuotientFilterElement(4, true,false,false);//3: 100
    start++;
    start++;
    start++;
    start++;
    *start = QuotientFilterElement(1, true,false,false); //7:225

    std::cout << g.query(226) << "\n";
    std::cout << g.query(66) << "\n";
}

int main () {
    std::cout << "Started Delete" << "\n";
    testQuery();
    std::cout << "Ended Delete" << "\n";

}