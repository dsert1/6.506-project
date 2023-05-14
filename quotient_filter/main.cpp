#include "quotient_filter.h"
// #include "quotient_filter_element.h"
// #include <boost/python.hpp>
#include <bitset>
#include <iostream>
#include <chrono>
#include <random>
#include <fstream>
#include <iostream>

int identity(int x) {
    return x;
}

int qfvSmall(int q, int r) {
  return ((q & 15) << 28) + r;
}

// Builds a value to insert into the filter, based on q and r
// q should be less than 16
int qfvLarge(int q, int r) {
  return ((q & 28) << 4) + r;
}

int generate_random_number(int min, int max) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(min, max);
  return dis(gen);
}
void test1() {
    QuotientFilter qf = QuotientFilter(20, &identity);
    std::cout << "TABLE CREATED\n"; 
    const int filter_capacity = qf.table_size;
    const int fill_limit = filter_capacity * 0.05;
    int remainder_max = (1 << qf.r);
    std::cout << "ARRAY CREATED with size: "<< fill_limit <<"\n";
    int numbersToInsert[fill_limit]; 

    for (int i = 0; i < fill_limit; i++) {
        int test_bucket = generate_random_number(0, qf.table_size);
        int test_remainder = generate_random_number(0, remainder_max);
        numbersToInsert[i] = qfvLarge(test_bucket, test_remainder);
    }

    // Insert elements until the filter is 5% filled
    for (int i = 0; i < fill_limit; i++) {
        qf.insertElement(numbersToInsert[i]);
    }
}

int main () {
    std::cout << "Started Delete" << "\n";
    test1();
    std::cout << "Ended Delete" << "\n";

}