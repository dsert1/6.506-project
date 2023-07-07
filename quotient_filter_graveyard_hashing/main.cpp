#include "quotient_filter_graveyard_hashing.h"
// #include "quotient_filter_element.h"
// #include <boost/python.hpp>
#include <iostream>
#include <bitset>
#include <fstream>
#include <chrono>
#include <random>
#include <assert.h>
#pragma warning( disable: 4838 )
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
    QuotientFilterGraveyard qf = QuotientFilterGraveyard(4, &identity, between_runs);
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

void testRedistributeOne(){
  QuotientFilterGraveyard qf = QuotientFilterGraveyard(4, &identity, between_runs);

  int buckets[] = {1, 3, 4, 6, 3, 6, 1, 3};
  int remainders[] = {100, 100, 100, 10, 14, 17, 103, 77};
    for (int i = 0; i < 8; i++) {
      qf.insertElement(qfv(buckets[i], remainders[i]));
    }
  
  
  int occupied_buckets[] = {1, 3, 4, 6};
  int curr_exc = 0;
  // for (int i=0; i < 16; i++) {
  //   if (curr_exc < 4 && occupied_buckets[curr_exc] == i) {
  //     std::cout << "EXPECT 1 GOT " << qf.table[i].is_occupied << "\n";
  //     curr_exc++;
  //     continue;
  //   }
  //     std::cout << "EXPECT 0 GOT " << qf.table[i].is_occupied << "\n";
  // }
  int numToDelete[] = {qfv(1, 100), qfv(1,103), qfv(3,100), qfv(3,14), qfv(3,77)};
  // qf.deleteElement(qfv(3,100));
  // qf.deleteElement(qfv(3,14));
  // int numToDelete = qfv(4,100);
  // qf.deleteElement(numToDelete);
  for (int i=0; i < 5; i++) {
    qf.deleteElement(numToDelete[i]);
  }

  int tombStones[] = {1,2,3,4,5};
  for (int i =0; i< 2;i++) {
    std::cout << "EXPECT 1 GOT " << qf.table[tombStones[i]].isTombstone << "\n";
    PredSucPair res = qf.decodeValue(qf.table[tombStones[i]].value);
    std::cout << "PREDECESSOR FOR TOMBSTONE AT " << tombStones[i] << " IS " << res.predecessor << "\n";
    std::cout << "SUCCESSOR FOR TOMBSTONE AT " << tombStones[i] << " IS " << res.successor << "\n";
  }
  // for (int i=6; i<16; i++) {
  //   std::cout << "EXPECT 0 GOT " << qf.table[i+1].isTombstone << "\n";
  // }

}


void testRedistributeTwo(){
  QuotientFilterGraveyard qf = QuotientFilterGraveyard(4, &identity, between_runs);

  int buckets[] = {1, 3, 4, 6, 3, 6, 1, 3};
  int remainders[] = {100, 100, 100, 10, 14, 17, 103, 77};
    for (int i = 0; i < 8; i++) {
      qf.insertElement(qfv(buckets[i], remainders[i]));
    }
  

  
  // qf.deleteElement(qfv(3,100));
  // qf.deleteElement(qfv(3,77));
  // qf.deleteElement(qfv(3,14));
  // int count = 0;
  // std::cout << "Before redistribution" << "\n";

  // std::cout << "Cluster start: " << qf.findClusterStart(7) << "\n";

  // for (int i = 1; i < 13; i++) {
  //   std::cout << "-----------START-----------" << "\n";
  //   std::cout << "GOT " << qf.table[i].isTombstone << "\n";
  //   std::cout << "GOT " << qf.table[i].isEndOfCluster << "\n";
  //   std::cout << "GOT " << qf.table[i].value << "\n";
  //   std::cout << "GOT " << qf.table[i].is_occupied << "\n";
  //   std::cout << "GOT " << qf.table[i].is_continuation << "\n";
  //   std::cout << "GOT " << qf.table[i].is_shifted << "\n";
  //   std::cout << "-----------FINISH-----------" << "\n";
  // }
  qf.redistributeTombstonesBetweenRunsInsert();

  // for (int i = 1; i < 13; i++) {
  //   std::cout << "-----------START-----------" << "\n";
  //   std::cout << "GOT " << qf.table[i].isTombstone << "\n";
  //   std::cout << "GOT " << qf.table[i].isEndOfCluster << "\n";
  //   std::cout << "GOT " << qf.table[i].value << "\n";
  //   std::cout << "GOT " << qf.table[i].is_occupied << "\n";
  //   std::cout << "GOT " << qf.table[i].is_continuation << "\n";
  //   std::cout << "GOT " << qf.table[i].is_shifted << "\n";
  //   std::cout << "-----------FINISH-----------" << "\n";
  // }

  // int occupied_buckets[] = {1, 3, 4, 6};
  // int curr_exc = 0;
  // for (int i=0; i < 16; i++) {
  //   if (curr_exc < 4 && occupied_buckets[curr_exc] == i) {
  //     std::cout << "EXPECT 1 GOT " << qf.table[i].is_occupied << "\n";
  //     curr_exc++;
  //     continue;
  //   }
  //     std::cout << "EXPECT 0 GOT " << qf.table[i].is_occupied << "\n";
  // }
  // int numToDelete[] = {qfv(1, 100), qfv(1,103), qfv(3,100), qfv(3,14), qfv(3,77)};
  // qf.deleteElement(qfv(3,100));
  // qf.deleteElement(qfv(3,14));
  // int numToDelete = qfv(4,100);
  // qf.deleteElement(numToDelete);
  // for (int i=0; i < 5; i++) {
  //   qf.deleteElement(numToDelete[i]);
  // }

  // int tombStones[] = {1,2,3,4,5};
  // for (int i =0; i< 2;i++) {
  //   std::cout << "EXPECT 1 GOT " << qf.table[tombStones[i]].isTombstone << "\n";
  //   std::cout << "EXPECT 1 GOT " << qf.table[tombStones[i]].isEndOfCluster << "\n";
  //   PredSucPair res = qf.decodeValue(qf.table[tombStones[i]].value);
  //   std::cout << "PREDECESSOR FOR TOMBSTONE AT " << tombStones[i] << " IS " << res.predecessor << "\n";
  //   std::cout << "SUCCESSOR FOR TOMBSTONE AT " << tombStones[i] << " IS " << res.successor << "\n";
  // }
  // for (int i=6; i<16; i++) {
  //   std::cout << "EXPECT 0 GOT " << qf.table[i+1].isTombstone << "\n";
  // }

}

void testEncode() {
    QuotientFilterGraveyard g = QuotientFilterGraveyard(3, &identity, between_runs);
    long long res = g.encodeValue(32, 64);
    PredSucPair res2 = g.decodeValue(res);
    std::cout << "Predecessor: " << res2.predecessor << "Successor: " << res2.successor << "\n";
}

int generate_random_number(int min, int max) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(min, max);
  return dis(gen);
}

void test2() {
  QuotientFilterGraveyard qf = QuotientFilterGraveyard(10, &identity, amortized_clean);
  int elementsInserted[qf.table_size];
  for (int i = 0; i < qf.table_size; i++) {
    elementsInserted[i] = generate_random_number(0,10000);
    qf.insertElement(elementsInserted[i]);
  }

  for (int i=0; i < qf.table_size; i++) {
      assert(qf.query(elementsInserted[i]) == true);
  }

  for (int i=0; i < qf.table_size; i++) {
    std::cout << "Deleted " << i << "\n";
    qf.deleteElement(elementsInserted[i]);
  }
    for (int i=0; i < qf.table_size; i++) {
      assert(qf.query(elementsInserted[i]) == false);
  }
}

//   for (int i=0; i<6; i++) {
//     std::cout<< "PRINTING OUT INFO AT: " << i << "\n";
//     if (qf.table[i].isTombstone) {
//       PredSucPair res = qf.decodeValue(qf.table[i].value);
//       std::cout << "PREDECESSOR: " <<res.predecessor << "\n";
//       std::cout << "SUCCESSOR: " <<res.successor << "\n";
//     } else {
//        std::cout << qf.table[i].value << "\n";
//     }
//     std::cout << "IS OCCUPIED: " <<qf.table[i].is_occupied << "\n";
//     std::cout << "IS SHIFTED: " <<qf.table[i].is_shifted << "\n";
//     std::cout << "IS CONTINUATION: " <<qf.table[i].is_continuation << "\n";
//   }
// }


int main() {
    test2();
    // std::hash<int> intHash;
    // uint64_t hashVal = intHash(40);
    // std::cout << hashVal << "\n";
}