#include "../quotient_filter/quotient_filter.h"
#include <bitset>
#include <iostream>
#include <chrono>
#include <random>
#include <fstream>
#include <thread>
#include <functional>
// disables a warning for converting ints to uint64_t
#pragma warning( disable: 4838 )

const int DURATION = 60;
const double MAX_FULLNESS = 0.9;

// To be used as the hash function for testing
int identity(int x) {
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

// // Fixture classes
// class QuotientFilterTest : public ::testing::Test {
//   protected:
//     QuotientFilter* qf; 

//     void SetUp() override {
//       qf = new QuotientFilter(5, &identity);
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
void perfTestInsert(QuotientFilter *qf) {
  std::cout << "Reached perfTestInsert" << std::endl;
  // Open output file
  std::ofstream outfile("normal_perfInsert.txt");

  int counter = 0;
  float currentFullness = 0.05;
  while (currentFullness <= MAX_FULLNESS) {
    if (counter % 1 == 0) {
      std::cout << "TestInsert Current iteration: " << counter << " Current fullness: " << currentFullness << std::endl;
    }
    // Calculate the number of elements to insert until the filter is 5% filled
    const int filter_capacity = qf->table_size;
    const int fill_limit = filter_capacity * 0.05;
    std::vector<int> numbersToInsert(fill_limit);

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
    auto insert_time = std::chrono::duration_cast<std::chrono::microseconds>(end_inserts - start_inserts).count();
    outfile << "Current Fullness: " << currentFullness << " Number inserted: " << fill_limit << " Time taken: " << insert_time << " microseconds" << std::endl;

    // perform queries for 60%
    auto start = std::chrono::high_resolution_clock::now();
    auto end = start + std::chrono::seconds(DURATION);
    

    int counter = 0;
    while (std::chrono::high_resolution_clock::now() < end) {
        qf->query(generate_random_number(0, qf->table_size));
        counter++;
    }

    // Write the random lookup values to file
    outfile << counter << " random queries in 60 seconds" << std::endl;

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
    outfile << counter2 << " successful queries in 60 seconds" << std::endl;
    currentFullness += 0.05;
    counter += 1;
    outfile << "-------" << std::endl;
  }
  

  // Close the file
  outfile.close();

  std::cout << "Finished perfTestInsert" << std::endl;
}

// TEST_F(QuotientFilterTest, PerfDelete) {
void perfTestDelete(QuotientFilter *qf) {
  std::cout << "Reached perfTestDelete" << std::endl;
  const int filter_capacity = qf->table_size;
  const int fill_limit = filter_capacity * MAX_FULLNESS;
  int remainder_max = (1 << qf->r);
  std::vector<int> numbersToInsert(fill_limit);
  // generates numbers to insert into filter
  for (int i = 0; i < fill_limit; i++) {
    numbersToInsert[i] = generate_random_number(0, qf->table_size);
  }

  // fill filter with values to delete!
  for (int i = 0; i < fill_limit; i++) {
    qf->insertElement(numbersToInsert[i]);
  }

  // Open output file
  std::ofstream outfile("perfDelete_refactored1.txt");

  int counter = 0;
  float currentFullness = MAX_FULLNESS;
  while (currentFullness >= 0.5) {
    if (counter % 1 == 0) {
      std::cout << "TestDelete Current iteration: " << counter << " Current fullness: " << currentFullness << std::endl;
    }


    // Calculate the number of elements to insert until the filter is 5% filled
    const int filter_capacity = qf->table_size;
    int remainder_max = (1 << qf->r);

    // Insert elements until the filter is 5% filled
    int deletePosition = currentFullness * filter_capacity - 1; // how far into the array we're at
    int deleteMinPosition = deletePosition - 0.05 * filter_capacity;
    if (deleteMinPosition < 0) {
      break;
    }

    auto start_inserts = std::chrono::steady_clock::now();
    std::cout << "Starting deletes... starting queries\n";
    for (int i = deletePosition; i > deleteMinPosition; i--) {
      qf->deleteElement(numbersToInsert[i]);
    }
    std::cout << "Ending deletes... starting queries\n";
    auto end_inserts = std::chrono::steady_clock::now();
    auto insert_time = std::chrono::duration_cast<std::chrono::microseconds>(end_inserts - start_inserts).count();
    outfile << "Current Fullness: " << currentFullness << ". Number deleted: " << fill_limit << " Time taken: " << insert_time << " microseconds" << std::endl;
    // outfile << "Current Fullness: " << currentFullness << ". Number deleted: " << delete_limit << " Time taken: " << delete_time << " microseconds" << std::endl;

    // perform queries for 60%
    auto start = std::chrono::high_resolution_clock::now();
    auto end = start + std::chrono::seconds(DURATION);
    

    int counter = 0;
    while (std::chrono::high_resolution_clock::now() < end) {
        qf->query(generate_random_number(0, qf->table_size));
        counter++;
    }
    // std::cout << "Finished all queries\n";
    // Write the random lookup values to file
    outfile << " " << counter << " random queries in 60 seconds" << std::endl;

    // list of random values we insert

    // perform queries for 60%
    auto start2 = std::chrono::high_resolution_clock::now();
    auto end2 = start2 + std::chrono::seconds(DURATION);
    

    int counter2 = 0;
    // std::cout << "deletePosition" << deletePosition << "deleteMinPosition" << deleteMinPosition << "\n";
    while (std::chrono::high_resolution_clock::now() < end2) {
        int num = generate_random_number(0, deleteMinPosition);
        // std::cout << "Size of array: " << numbersToInsert.size() << "Trying to access: " << num <<"\n";
        int randomValueToQuery = numbersToInsert[num];
        qf->query(randomValueToQuery);
        counter2++;
    }

    // Write the random lookup values to file
    outfile << " " << counter2 << " successful queries in 60 seconds" << std::endl;
    currentFullness -= 0.05;
    counter += 1;
  }
  

  // Close the file
  outfile.close();
  std::cout << "Finished perfTestDelete" << std::endl;
}

// TEST_F(QuotientFilterTest, PerfMixed) {
  void perfTestMixed(QuotientFilter *qf) {
  std::cout << "Reached perfTestMixed" << std::endl;
  // Open output file
  std::ofstream outfile("perfMixed_refactored.txt");
  outfile << "Reached perfTestMixed" << std::endl;

  int counter = 0;
  float currentFullness = 0.5;
  while (currentFullness <= MAX_FULLNESS) {
    if (counter % 1 == 0) {
      std::cout << "Current iteration: " << counter << " Current fullness: " << currentFullness << std::endl;
    }

    // Calculate the number of elements to insert until the filter is 5% filled
    const int filter_capacity = qf->table_size;
    const int fill_limit = filter_capacity * 0.1;
    const int delete_limit = filter_capacity * 0.05;
    std::vector<int> numbersToInsert(fill_limit);

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
    auto insert_time = std::chrono::duration_cast<std::chrono::microseconds>(end_inserts - start_inserts).count();
    outfile << "testMixed Current Fullness: " << currentFullness << ". Number inserted: " << fill_limit << " Time taken: " << insert_time << " microseconds" << std::endl;

    std::cout << "Starting deletes... starting queries\n";
    // Delete elements until the filter is 5% filled
    auto start_deletes = std::chrono::steady_clock::now();
    for (int i = 0; i < delete_limit; i++) {
      qf->deleteElement(numbersToInsert[fill_limit - i - 1]);
    }

    std::cout << "Finished all deletes... starting queries\n";
    auto end_deletes = std::chrono::steady_clock::now();
    auto delete_time = std::chrono::duration_cast<std::chrono::microseconds>(end_deletes - start_deletes).count();
    // outfile << "Current Fullness: " << currentFullness << ". Deletion " << delete_limit << " " << delete_time << " microseconds" << std::endl;
    outfile << "Current Fullness: " << currentFullness << ". Number deleted: " << delete_limit << " Time taken: " << delete_time << " microseconds" << std::endl;


    // perform queries for 60%
    auto start = std::chrono::high_resolution_clock::now();
    auto end = start + std::chrono::seconds(DURATION);
    
    int counter = 0;
    while (std::chrono::high_resolution_clock::now() < end) {
        qf->query(generate_random_number(0, qf->table_size));
        counter++;
    }
    std::cout << "Finished all queries\n";

    // Write the random lookup values to file
    outfile << " " << counter << " random queries in 60 seconds" << std::endl;

    // list of random values we insert

    // perform queries for 60%
    auto start2 = std::chrono::high_resolution_clock::now();
    auto end2 = start2 + std::chrono::seconds(DURATION);
    
    std::cout << "Starting all queries\n";
    int counter2 = 0;
    while (std::chrono::high_resolution_clock::now() < end2) {
        int randomValueToQuery = numbersToInsert[generate_random_number(0, fill_limit / 2)];
        qf->query(randomValueToQuery);
        counter2++;
    }

    // Write the random lookup values to file
    outfile << " " << counter2 << " successful queries in 60 seconds" << std::endl;
    currentFullness += 0.05;
    counter += 1;
  }
  

  // Close the file
  outfile.close();

  std::cout << "Finished perfTestMixed" << std::endl;
}

int main(int argc, char **argv) {
    QuotientFilter qf = QuotientFilter(20, &identity);
    perfTestInsert(&qf);
    // perfTestDelete(&qf);
    // perfTestMixed(&qf);
}