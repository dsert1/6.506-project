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

const int DURATION = 10;
const double MAX_FULLNESS = 0.65;
const int FILTER_Q = 15;

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

// Testing performance while filling in increments of 5
void perfTestInsert(QuotientFilterGraveyard* qf, std::string test_prefix) {
  // Open output file
  std::ofstream outfile(test_prefix + "_perfInsert.txt");

  float currentFullness = 0.05;
  while (currentFullness <= MAX_FULLNESS) {
    std::cout << "Current fullness: " << currentFullness << std::endl;
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
    outfile << counter << " random queries in " << DURATION << " seconds" << std::endl;

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
    outfile << counter2 << " successful queries in " << DURATION << " seconds" << std::endl;
    outfile << "-------" << std::endl;
    currentFullness += 0.05;
  }
  

  // Close the file
  outfile.close();
}

// TEST_F(QuotientFilterGraveyardTest, PerfDelete) {
void perfTestDelete(QuotientFilterGraveyard* qf, std::string test_prefix) {
  // Open output file
  std::ofstream outfile(test_prefix + "_perfDelete.txt");
  
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

  float currentFullness = MAX_FULLNESS;
  while (currentFullness >= 0.05) {
    std::cout << " Current fullness: " << currentFullness << std::endl;
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
    for (int i = deletePosition; i > deleteMinPosition; i--) {
      qf->deleteElement(numbersToInsert[i]);
    }
    auto end_inserts = std::chrono::steady_clock::now();
    auto insert_time = std::chrono::duration_cast<std::chrono::microseconds>(end_inserts - start_inserts).count();
    outfile << "Current Fullness: " << currentFullness << " Number deleted: " << fill_limit << " Time taken: " << insert_time << " microseconds" << std::endl;

    // perform queries for 60%
    auto start = std::chrono::high_resolution_clock::now();
    auto end = start + std::chrono::seconds(DURATION);
    

    int counter = 0;
    while (std::chrono::high_resolution_clock::now() < end) {
        qf->query(generate_random_number(0, qf->table_size));
        counter++;
    }

    // Write the random lookup values to file
    outfile << " " << counter << " random queries in " << DURATION << " seconds" << std::endl;

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
    outfile << " " << counter2 << " successful queries in " << DURATION << " seconds" << std::endl;
    outfile << "-------" << std::endl;
    currentFullness -= 0.05;
  }
  

  // Close the file
  outfile.close();
}

// TEST_F(QuotientFilterGraveyardTest, PerfMixed) {
void perfTestMixed(QuotientFilterGraveyard* qf, std::string test_prefix) {
  // Open output file
  std::ofstream outfile(test_prefix + "_perfMixed.txt");

  float currentFullness = 0.05;
  while (currentFullness <= MAX_FULLNESS - 0.05) {
    std::cout << " Current fullness: " << currentFullness << std::endl;
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
    auto insert_time = std::chrono::duration_cast<std::chrono::microseconds>(end_inserts - start_inserts).count();
    outfile << "Current Fullness: " << currentFullness << " Number inserted: " << fill_limit << " Time taken: " << insert_time << " microseconds" << std::endl;


    // Delete elements until the filter is 5% filled
    auto start_deletes = std::chrono::steady_clock::now();
    for (int i = 0; i < delete_limit; i++) {
      qf->deleteElement(numbersToInsert[fill_limit - i - 1]);
    }

    auto end_deletes = std::chrono::steady_clock::now();
    auto delete_time = std::chrono::duration_cast<std::chrono::microseconds>(end_deletes - start_deletes).count();
    // outfile << "Current Fullness: " << currentFullness << ". Deletion " << delete_limit << " " << delete_time << " microseconds" << std::endl;
    outfile << "Current Fullness: " << currentFullness << " Number deleted: " << delete_limit << " Time taken: " << delete_time << " microseconds" << std::endl;

    // perform queries for 60%
    auto start = std::chrono::high_resolution_clock::now();
    auto end = start + std::chrono::seconds(DURATION);
    

    int counter = 0;
    while (std::chrono::high_resolution_clock::now() < end) {
        qf->query(generate_random_number(0, qf->table_size));
        counter++;
    }

    // Write the random lookup values to file
    outfile << " " << counter << " random queries in " << DURATION << " seconds" << std::endl;

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
    outfile << " " << counter2 << " successful queries in " << DURATION << " seconds" << std::endl;
    outfile << "-------" << std::endl;
    currentFullness += 0.05;
  }
  

  // Close the file
  outfile.close();
}

int main(int argc, char **argv) {
    // QuotientFilterGraveyard qfi = QuotientFilterGraveyard(FILTER_Q, &hash_fn, evenly_distribute);
    // perfTestInsert(&qfi, "graveyard_evenlydist");

    // QuotientFilterGraveyard qfd = QuotientFilterGraveyard(FILTER_Q, &hash_fn, evenly_distribute);
    // perfTestDelete(&qfd, "graveyard_evenlydist");

    QuotientFilterGraveyard qfm = QuotientFilterGraveyard(FILTER_Q, &hash_fn, evenly_distribute);
    perfTestMixed(&qfm, "graveyard_evenlydist");

    // QuotientFilterGraveyard qf1 = QuotientFilterGraveyard(20, &hash_fn, no_redistribution);
    // QuotientFilterGraveyard qf2 = QuotientFilterGraveyard(20, &hash_fn, between_runs);
    // QuotientFilterGraveyard qf3 = QuotientFilterGraveyard(FILTER_Q, &hash_fn, between_runs_insert);
    // //QuotientFilterGraveyard qf4 = QuotientFilterGraveyard(15, &hash_fn, evenly_distribute);
    // perfTestInsert(&qf3);
    // perfTestInsert(&qf2);
    // perfTestInsert(&qf3);
    // perfTestInsert(&qf4);
    // perfTestDelete(&qf);
    // perfTestMixed(&qf);
}
