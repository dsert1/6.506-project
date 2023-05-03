#include "quotient_filter_graveyard_hashing.h"
// #include "quotient_filter_element.h"
// #include <boost/python.hpp>
#include <iostream>

int identity(int x) {
    return x;
}

// void testDelete() {
//     QuotientFilterGraveyard g = QuotientFilterGraveyard(3, &identity);
//     QuotientFilterElement* start = g.table;
//     int elements[10] = {43,58,100,229,225};
//     *start = QuotientFilterElement(5,false,true,true); //0: 229
//     start++;
//     *start = QuotientFilterElement(26,true,false,false); //1: 58
//     start++;
//     *start = QuotientFilterElement(11,false,true,true); //2: 43
//     start++;
//     *start = QuotientFilterElement(4, true,false,false);//3: 100
//     start++;
//     start++;
//     start++;
//     start++;
//     *start = QuotientFilterElement(1, true,false,false); //7:225
//     // g.table_size = 5;
//     g.deleteElement(225);
//     g.deleteElement(58);
//     // g.deleteElement(20);
//     QuotientFilterElement* newStart = g.table;
//     int count = 0;
//     while (count < g.table_size){
//         std::cout << "index: " << count << "\n";
//         std::cout << "fr: " <<(*newStart).value << "\n";
//         std::cout << "is_occupied: " <<(*newStart).is_occupied << "\n";
//         std::cout << "is_continuation: " <<(*newStart).is_continuation << "\n";
//         std::cout << "is_shifted: " <<(*newStart).is_shifted << "\n";
//         std::cout << "is_tombstone: " <<(*newStart).isTombstone << "\n";
//         if ((*newStart).isTombstone) {
//             PredSucPair res2 = g.decodeValue((*newStart).value);
//             std::cout << "Predecessor: " << res2.predecessor << "Successor: " << res2.successor << "\n";
//         }
//         std::cout << "--------" << "\n";
//         newStart++;
//         count++;
//     }
// }

int qfv(int q, int r) {
  return ((q & 15) << 28) + r;
}

void assert_empty_buckets(QuotientFilterGraveyard* qf, int exceptionCount, int exceptions[]) {
  int ce = 0;
  for (int bucket = 0; bucket < 16; bucket++) {
    if (ce < exceptionCount && exceptions[ce] == bucket) {
      ce++;
      continue;
    }
    // EXPECT_FALSE(qf->query(qfv(bucket, 0)));
//     if (validateTable) {
      QuotientFilterElement elt = qf->table[bucket];
      std::cout << elt.is_occupied << "\n";
      std::cout << elt.is_shifted<< "\n";
      std::cout << elt.is_continuation << "\n";
      // EXPECT_EQ(elt.value, 0);
//     }
  }
}

void testQuery() {
    QuotientFilterGraveyard qf = QuotientFilterGraveyard(4, &identity);
    int first_bucket = 0;
    int remainders[] = {314, 159, 265, 358};

    int second_bucket = (first_bucket + 1) % 16;
    int buckets[] = {first_bucket, first_bucket, second_bucket, second_bucket};
    int rb2 = (first_bucket + 1) % 16;
    int rb3 = (first_bucket + 2) % 16;

    for (int i = 0; i < 4; i++) {
      qf.insertElement(qfv(buckets[i], remainders[i]));
      // std::cout << "GOT RESULT: " << qf.query(qfv(buckets[i], remainders[i])) << "\n";
    }

    qf.deleteElement(qfv(buckets[0], remainders[0]));
    qf.deleteElement(qfv(buckets[1], remainders[1]));
    // for (int i = 0; i < 4; i++) {
    //   std::cout << "GOT RESULT: " << qf.query(qfv(buckets[i], remainders[i])) << "\n";
    // }
    std::cout << "GOT RESULT: " << qf.query(qfv(buckets[0], 0)) << "\n";
    // for (int bucket = 0; bucket < 16; bucket++) {
    //     uint64_t val = ((bucket&15)<<28)+28;
    //     std::bitset<32> y(((bucket&15)<<28)+28);
    //     std::cout << y << '\n';
    //     std::cout << "QUERYING FOR " << val << "\n";
    //     std::cout << "GOT RESULT: " << qf.query(val) << "\n";
    // }
    // QuotientFilterGraveyard g = QuotientFilterGraveyard(3, &identity);
    // QuotientFilterElement* start = g.table;
    // int elements[10] = {43,58,100,229,225};
    // *start = QuotientFilterElement(5,false,true,true); //0: 229
    // start++;
    // *start = QuotientFilterElement(26,true,false,false); //1: 58
    // start++;
    // *start = QuotientFilterElement(11,false,true,true); //2: 43
    // start++;
    // *start = QuotientFilterElement(4, true,false,false);//3: 100
    // start++;
    // start++;
    // start++;
    // start++;
    // *start = QuotientFilterElement(1, true,false,false); //7:225
    // // g.table_size = 5;
    // // std::cout << g.query(229) << "\n";
    // // std::cout << g.query(56) << "\n";
    // // std::cout << g.query(58) << "\n";
    // g.deleteElement(58);
    // std::cout << g.query(43) << "\n";
    // // g.deleteElement(58);

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