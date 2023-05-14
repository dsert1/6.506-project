#include "../quotient_filter_graveyard_hashing/quotient_filter_graveyard_hashing.h"
#include <bitset>
#include <iostream>
#include <chrono>
#include <random>
#include <fstream>
#include <thread>
#include <functional>
// disables a warning for converting ints to uint64_t
#pragma warning( disable: 4838 )

const int DURATION = 1;
const double MAX_FULLNESS = 0.2;

// To be used as the hash function for testing
int hash_fn(int x) {
    std::hash<int> hash_fn;
    size_t hash_value = hash_fn(x);
    return hash_value;
}

// Function to generate a random number between a range
int generate_random_number(int min, int max) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(min, max);
  return dis(gen);
}

bool validateTable;

// Fixture class
// class GraveyardFilterTest : public ::testing::Test {
//   protected:
//     QuotientFilterGraveyard* qf;

//     void SetUp() override {
//       qf = new QuotientFilterGraveyard(10, &hash_fn, no_redistribution);
//     }

//     // void TearDown() override {}
// };

// Builds a value to insert into the filter, based on q and r
// q should be less than 16
int qfvSmall(int q, int r) {
  return ((q & 15) << 28) + r;
}

// Builds a value to insert into the filter, based on q and r
// q should be less than 16
int qfv(int q, int r) {
  return ((q & 28) << 4) + r;
}

// Testing performance with uniform random lookups on a 5% filled element
// TEST_F(QuotientFilterTest, PerfInsertion) {
void perfTestInsert(QuotientFilterGraveyard* qf) {
  // Open output file
  std::ofstream outfile("perfInsert_graveyard_noredis.txt");

  float currentFullness = 0.05;
  while (currentFullness <= MAX_FULLNESS) {
    // Calculate the number of elements to insert until the filter is 5% filled
    const int filter_capacity = qf->table_size;
    const int fill_limit = filter_capacity * 0.05;
    int numbersToInsert[fill_limit];

    // generates numbers to insert into filter
    for (int i = 0; i < fill_limit; i++) {
      numbersToInsert[i] = generate_random_number(0, qf->table_size);
    }

    // Insert elements until the filter is 5% filled
    auto start_inserts = std::chrono::steady_clock::now();
    for (int i = 0; i < fill_limit; i++) {
      qf->insertElement(numbersToInsert[i]);
    }
    auto end_inserts = std::chrono::steady_clock::now();
    auto insert_time = std::chrono::DURATION_cast<std::chrono::microseconds>(end_inserts - start_inserts).count();
    outfile << "Current Fullness: " << currentFullness << ". Insertion " << fill_limit << " " << insert_time << " microseconds" << std::endl;

    // perform queries for 60%
    auto start = std::chrono::high_resolution_clock::now();
    auto end = start + std::chrono::seconds(DURATION);
    

    int counter = 0;
    while (std::chrono::high_resolution_clock::now() < end) {
        qf->query(generate_random_number(0, qf->table_size));
        counter++;
    }

    // Write the random lookup values to file
    outfile << " " << counter << " random queries in 60 seconds" << std::endl;

    // list of random values we insert

    // perform queries for 60%
    auto start2 = std::chrono::high_resolution_clock::now();
    auto end2 = start2 + std::chrono::seconds(DURATION);
    

    int counter2 = 0;
    while (std::chrono::high_resolution_clock::now() < end2) {
        int randomValueToQuery = numbersToInsert[generate_random_number(0, fill_limit)];
        qf->query(randomValueToQuery);
        counter2++;
    }

    // Write the random lookup values to file
    outfile << " " << counter2 << " successful queries in 60 seconds" << std::endl;
    currentFullness += 0.05;
  }
  

  // Close the file
  outfile.close();
}

// TEST_F(QuotientFilterTest, PerfDelete) {
void perfTestDelete(QuotientFilterGraveyard* qf) {
  const int filter_capacity = qf->table_size;
  const int fill_limit = filter_capacity * MAX_FULLNESS;
  int numbersToInsert[fill_limit];
  // generates numbers to insert into filter
  for (int i = 0; i < fill_limit; i++) {
    numbersToInsert[i] = generate_random_number(0, qf->table_size);
  }

  // fill filter with values to delete!
  for (int i = 0; i < fill_limit; i++) {
    qf->insertElement(numbersToInsert[i]);
  }

  // Open output file
  std::ofstream outfile("perfDelete_graveyard_noredis.txt");

  float currentFullness = MAX_FULLNESS;
  while (currentFullness >= 0.05) {
    // Calculate the number of elements to insert until the filter is 5% filled
    const int filter_capacity = qf->table_size;
    int remainder_max = (1 << qf->r);

    // Insert elements until the filter is 5% filled
    int deletePosition = currentFullness * filter_capacity - 1; // how far into the array we're at
    int deleteMinPosition = deletePosition - 0.05 * filter_capacity;
    auto start_inserts = std::chrono::steady_clock::now();
    for (int i = deletePosition; i > deleteMinPosition; i--) {
      qf->deleteElement(numbersToInsert[i]);
    }
    auto end_inserts = std::chrono::steady_clock::now();
    auto insert_time = std::chrono::DURATION_cast<std::chrono::microseconds>(end_inserts - start_inserts).count();
    outfile << "Current Fullness: " << currentFullness << ". Deletion " << fill_limit << " " << insert_time << " microseconds" << std::endl;

    // perform queries for 60%
    auto start = std::chrono::high_resolution_clock::now();
    auto end = start + std::chrono::seconds(DURATION);
    

    int counter = 0;
    while (std::chrono::high_resolution_clock::now() < end) {
        qf->query(generate_random_number(0, qf->table_size));
        counter++;
    }

    // Write the random lookup values to file
    outfile << " " << counter << " random queries in 60 seconds" << std::endl;

    // list of random values we insert

    // perform queries for 60%
    auto start2 = std::chrono::high_resolution_clock::now();
    auto end2 = start2 + std::chrono::seconds(DURATION);
    

    int counter2 = 0;
    while (std::chrono::high_resolution_clock::now() < end2) {
        int randomValueToQuery = numbersToInsert[generate_random_number(0, deleteMinPosition)];
        qf->query(randomValueToQuery);
        counter2++;
    }

    // Write the random lookup values to file
    outfile << " " << counter2 << " successful queries in 60 seconds" << std::endl;
    currentFullness -= 0.05;
  }
  

  // Close the file
  outfile.close();
}

// TEST_F(QuotientFilterTest, PerfMixed) {
void perfTestMixed(QuotientFilterGraveyard* qf) {
  // Open output file
  std::ofstream outfile("perfMixed_graveyard_noredis.txt");

  float currentFullness = 0.05;
  while (currentFullness <= MAX_FULLNESS) {
    // Calculate the number of elements to insert until the filter is 5% filled
    const int filter_capacity = qf->table_size;
    const int fill_limit = filter_capacity * 0.1;
    const int delete_limit = filter_capacity * 0.05;
    int numbersToInsert[fill_limit];

    // generates numbers to insert into filter
    for (int i = 0; i < fill_limit; i++) {
      numbersToInsert[i] = generate_random_number(0, qf->table_size);
    }

    // Insert elements until the filter is 5% filled
    auto start_inserts = std::chrono::steady_clock::now();
    for (int i = 0; i < fill_limit; i++) {
      qf->insertElement(numbersToInsert[i]);
    }
    auto end_inserts = std::chrono::steady_clock::now();
    auto insert_time = std::chrono::DURATION_cast<std::chrono::microseconds>(end_inserts - start_inserts).count();
    outfile << "Current Fullness: " << currentFullness << ". Insertion " << fill_limit << " " << insert_time << " microseconds" << std::endl;


    // Delete elements until the filter is 5% filled
    auto start_deletes = std::chrono::steady_clock::now();
    for (int i = 0; i < delete_limit; i++) {
      qf->deleteElement(numbersToInsert[fill_limit - i - 1]);
    }

    auto end_deletes = std::chrono::steady_clock::now();
    auto delete_time = std::chrono::DURATION_cast<std::chrono::microseconds>(end_deletes - start_deletes).count();
    outfile << "Current Fullness: " << currentFullness << ". Deletion " << delete_limit << " " << delete_time << " microseconds" << std::endl;

    // perform queries for 60%
    auto start = std::chrono::high_resolution_clock::now();
    auto end = start + std::chrono::seconds(DURATION);
    

    int counter = 0;
    while (std::chrono::high_resolution_clock::now() < end) {
        qf->query(generate_random_number(0, qf->table_size));
        counter++;
    }

    // Write the random lookup values to file
    outfile << " " << counter << " random queries in 60 seconds" << std::endl;

    // list of random values we insert

    // perform queries for 60%
    auto start2 = std::chrono::high_resolution_clock::now();
    auto end2 = start2 + std::chrono::seconds(DURATION);
    

    int counter2 = 0;
    while (std::chrono::high_resolution_clock::now() < end2) {
        int randomValueToQuery = numbersToInsert[generate_random_number(0, fill_limit / 2)];
        qf->query(randomValueToQuery);
        counter2++;
    }

    // Write the random lookup values to file
    outfile << " " << counter2 << " successful queries in 60 seconds" << std::endl;
    currentFullness += 0.05;
  }
  

  // Close the file
  outfile.close();
}

int main(int argc, char **argv) {
    // QuotientFilter qf = QuotientFilter(5, &identity);
    QuotientFilterGraveyard qf = QuotientFilterGraveyard(10, &hash_fn, no_redistribution);
    perfTestInsert(&qf);
    perfTestDelete(&qf);
    perfTestMixed(&qf);
}
