#include "quotient_filter_graveyard_hashing.h"
// #include "quotient_filter_element.h"
// #include <boost/python.hpp>
#include <iostream>

int identity(int x) {
    return x;
}

void testDelete() {
    QuotientFilterGraveyard g = QuotientFilterGraveyard(3, &identity);
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
    g.deleteElement(58);
    // g.deleteElement(20);
    QuotientFilterElement* newStart = g.table;
    int count = 0;
    while (count < g.table_size){
        std::cout << "index: " << count << "\n";
        std::cout << "fr: " <<(*newStart).value << "\n";
        std::cout << "is_occupied: " <<(*newStart).is_occupied << "\n";
        std::cout << "is_continuation: " <<(*newStart).is_continuation << "\n";
        std::cout << "is_shifted: " <<(*newStart).is_shifted << "\n";
        std::cout << "is_tombstone: " <<(*newStart).isTombstone << "\n";
        if ((*newStart).isTombstone) {
            PredSucPair res2 = g.decodeValue((*newStart).value);
            std::cout << "Predecessor: " << res2.predecessor << "Successor: " << res2.successor << "\n";
        }
        std::cout << "--------" << "\n";
        newStart++;
        count++;
    }
}

void testQuery() {
    QuotientFilterGraveyard g = QuotientFilterGraveyard(3, &identity);
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
    // std::cout << g.query(229) << "\n";
    // std::cout << g.query(56) << "\n";
    // std::cout << g.query(58) << "\n";
    g.deleteElement(58);
    std::cout << g.query(43) << "\n";
    // g.deleteElement(58);

}

void testEncode() {
    QuotientFilterGraveyard g = QuotientFilterGraveyard(3, &identity);
    long long res = g.encodeValue(32, 64);
    PredSucPair res2 = g.decodeValue(res);
    std::cout << "Predecessor: " << res2.predecessor << "Successor: " << res2.successor << "\n";
}

int main() {
    testQuery();
}