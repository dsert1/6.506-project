#include <gtest/gtest.h>
#include "../quotient_filter/quotient_filter.h"
#include <bitset>
#include <iostream>
#include <chrono>
#include <random>
#include <fstream>
#include <thread>
// disables a warning for converting ints to uint64_t
#pragma warning( disable: 4838 )

// To be used as the hash function for testing
int identity(int x) {
    return x;
}

// Function to generate a random number between a range
int generate_random_number(int min, int max) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(min, max);
  return dis(gen);
}

bool validateTable;

// Fixture classes
class QuotientFilterTest : public ::testing::Test {
  protected:
    QuotientFilter* qf; 

    void SetUp() override {
      qf = new QuotientFilter(5, &identity);
    }

    // void TearDown() override {}
};

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
TEST_F(QuotientFilterTest, Perf5) {
  int duration = 1;
  double max_fullness = 0.2;
  // Open output file
  std::ofstream outfile("perf5_lookup_times.txt");

  float currentFullness = 0.05;
  while (currentFullness < max_fullness) {
    // Calculate the number of elements to insert until the filter is 5% filled
    const int filter_capacity = qf->table_size;
    const int fill_limit = filter_capacity * 0.05;
    int remainder_max = (1 << qf->r);
    int numbersToInsert[fill_limit];

    // generates numbers to insert into filter
    for (int i = 0; i < fill_limit; i++) {
      int test_bucket = generate_random_number(0, qf->table_size);
      int test_remainder = generate_random_number(0, remainder_max);
      numbersToInsert[i] = qfv(test_bucket, test_remainder);
    }

    // Insert elements until the filter is 5% filled
    int start_inserts = std::chrono::steady_clock::now();
    for (int i = 0; i < fill_limit; i++) {
      qf->insertElement(numbersToInsert[i]);
    }
    int end_inserts = std::chrono::steady_clock::now();
    int insert_time = std::chrono::duration_cast<std::chrono::microseconds>(end_inserts - start_inserts);
    outfile << "Current Fullness: " << currentFullness << ". Insertion " << fill_limit << " " << insert_time << " microseconds" << std::endl;

    // perform queries for 60%
    int start = std::chrono::high_resolution_clock::now();
    int end = start + std::chrono::seconds(duration);
    

    int counter = 0;
    while (std::chrono::high_resolution_clock::now() < end) {
        qf->query(generate_random_number(0, qf->table_size));
        counter++;
    }

    // Write the random lookup values to file
    outfile << " " << counter << " random queries in 60 seconds" << std::endl;

    // list of random values we insert

    // perform queries for 60%
    int start2 = std::chrono::high_resolution_clock::now();
    int end2 = start2 + std::chrono::seconds(duration);
    

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

// Testing performance with uniform random lookups on a 5% filled element
// TEST_F(QuotientFilterTestSmall , Perf5) {
//   // Open output file
//   std::ofstream outfile("perf5_lookup_times.txt");

//   // Calculate the number of elements to insert until the filter is 5% filled
//   const int filter_capacity = qf->table_size;
//   const int fill_limit = filter_capacity * 0.05;

//   int numbersToInsert[fill_limit];
//   for (int i = 0; i < fill_limit; i++) {
//     int test_bucket = generate_random_number(0, qf->table_size);
//     int test_remainder = generate_random_number(0, qf->table_size);
//     numbersToInsert[i] = qfv(test_bucket, test_remainder);
//   }

//   // Insert elements until the filter is 5% filled
//   int start_inserts = std::chrono::steady_clock::now();
//   for (int i = 0; i < fill_limit; i++) {
//     qf->insertElement(numbersToInsert[i]);
//   }
//   int end_inserts = std::chrono::steady_clock::now();
//   outfile << "Inserting : " << fill_limit << "numbers took" << uniform_random_lookup_time << " microseconds" << std::endl;

//   // Measure the time taken for uniform random lookups
//   int start_uniform_random_lookup = std::chrono::steady_clock::now();
//   const int uniform_random_lookup_count = 100000;
//   for (int i = 0; i < uniform_random_lookup_count; i++) {
//     int test_bucket = generate_random_number(0, filter_capacity - 1);
//     int test_remainder = generate_random_number(0, qf->table_size);
//     qf->query(qfv(test_bucket, test_remainder));
//   }
//   int end_uniform_random_lookup = std::chrono::steady_clock::now();
//   int uniform_random_lookup_time = std::chrono::duration_cast<std::chrono::microseconds>(end_uniform_random_lookup - start_uniform_random_lookup).count();

//   // Measure the time taken for successful lookups
//   int start_successful_lookup = std::chrono::steady_clock::now();
//   const int successful_lookup_count = 10000;
//   for (int i = 0; i < successful_lookup_count; i++) {
//     int test_bucket = generate_random_number(0, filter_capacity - 1);
//     int test_remainder = generate_random_number(0, qf->table_size);
//     qf->insertElement(qfv(test_bucket, test_remainder));
//     qf->query(qfv(test_bucket, test_remainder));
//   }
//   int end_successful_lookup = std::chrono::steady_clock::now();
//   int successful_lookup_time = std::chrono::duration_cast<std::chrono::microseconds>(end_successful_lookup - start_successful_lookup).count();

//   // Write the values to the file
//   outfile << "Uniform random lookup time: " << uniform_random_lookup_time << " microseconds" << std::endl;
//   outfile << "Successful lookup time: " << successful_lookup_time << " microseconds" << std::endl;


//   // Print the time taken for uniform random lookups and successful lookups
//   std::cout << "Uniform random lookup time: " << uniform_random_lookup_time << " microseconds" << std::endl;
//   std::cout << "Successful lookup time: " << successful_lookup_time << " microseconds" << std::endl;


//   // Close the file
//   outfile.close();
// }

// // Performance: to insert elements until 95% filled, then perform uniform random lookups and successful lookups
// TEST_F(QuotientFilterTest, Perf95) {

//   // Open output file
//   std::ofstream outfile("perf95_lookup_times.txt");

//   // Calculate the number of elements to insert until the filter is 95% filled
//   const int filter_capacity = qf->table_size;
//   const int fill_limit = filter_capacity * 0.95;

//   // Insert elements until the filter is 95% filled
//   for (int i = 0; i < fill_limit; i++) {
//     int test_bucket = generate_random_number(0, filter_capacity - 1);
//     int test_remainder = generate_random_number(0, qf->table_size);
//     qf->insertElement(qfv(test_bucket, test_remainder));
//   }

//   // Measure the time taken for uniform random lookups
//   int start_uniform_random_lookup = std::chrono::steady_clock::now();
//   const int uniform_random_lookup_count = 10000;
//   for (int i = 0; i < uniform_random_lookup_count; i++) {
//     int test_bucket = generate_random_number(0, filter_capacity - 1);
//     int test_remainder = generate_random_number(0, qf->table_size);
//     qf->query(qfv(test_bucket, test_remainder));
//   }
//   int end_uniform_random_lookup = std::chrono::steady_clock::now();
//   int uniform_random_lookup_time = std::chrono::duration_cast<std::chrono::microseconds>(end_uniform_random_lookup - start_uniform_random_lookup).count();

//   // Measure the time taken for successful lookups
//   int start_successful_lookup = std::chrono::steady_clock::now();
//   const int successful_lookup_count = 10000;
//   for (int i = 0; i < successful_lookup_count; i++) {
//     int test_bucket = generate_random_number(0, filter_capacity - 1);
//     int test_remainder = generate_random_number(0, qf->table_size);
//     qf->insertElement(qfv(test_bucket, test_remainder));
//     qf->query(qfv(test_bucket, test_remainder));
//   }
//   int end_successful_lookup = std::chrono::steady_clock::now();
//   int successful_lookup_time = std::chrono::duration_cast<std::chrono::microseconds>(end_successful_lookup - start_successful_lookup).count();


//   // Write the values to the file
//   outfile << "Uniform random lookup time: " << uniform_random_lookup_time << " microseconds" << std::endl;
//   outfile << "Successful lookup time: " << successful_lookup_time << " microseconds" << std::endl;


//   // Print the time taken for uniform random lookups and successful lookups
//   std::cout << "Uniform random lookup time: " << uniform_random_lookup_time << " microseconds" << std::endl;
//   std::cout << "Successful lookup time: " << successful_lookup_time << " microseconds" << std::endl;

//   // Close the file
//   outfile.close();
// }

// // Performance: to perform a mix of inserts, deletes, and lookups for a fixed amount of time
// TEST_F(QuotientFilterTest, PerfMixed) {

//   // Open output file
//   std::ofstream outfile("perfmixed.txt");

//   // Set the time duration of the test (in seconds)
//   const int test_duration = 10;

//   // Initialize variables for tracking the number of operations performed
//   int insert_count = 0;
//   int delete_count = 0;
//   int lookup_count = 0;

//   // Measure the time taken for the test
//   int start_time = std::chrono::steady_clock::now();
//   while (true) {
//     int current_time = std::chrono::steady_clock::now();
//     int elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
//     if (elapsed_time >= test_duration) {
//       break;
//     }

//     // Generate a random operation to perform (insert, delete, or lookup)
//     int operation_type = generate_random_number(0, 2);

//     // Perform the selected operation
//     if (operation_type == 0) { // Insert
//       int test_bucket = generate_random_number(0, qf->table_size);
//       int test_remainder = generate_random_number(0, qf->table_size);
//       qf->insertElement(qfv(test_bucket, test_remainder));
//       insert_count++;
//     } else if (operation_type == 1) { // Delete
//       int test_bucket = generate_random_number(0, qf->table_size);
//       int test_remainder = generate_random_number(0, qf->table_size);
//       qf->deleteElement(qfv(test_bucket, test_remainder));
//       delete_count++;
//     } else { // Lookup
//       int test_bucket = generate_random_number(0, qf->table_size);
//       int test_remainder = generate_random_number(0, qf->table_size);
//       qf->query(qfv(test_bucket, test_remainder));
//       lookup_count++;
//     }
//   }

//   // Measure the time taken for each operation
//   int total_operation_count = insert_count + delete_count + lookup_count;
//   int insert_time = (double)insert_count / total_operation_count * test_duration * 1000;
//   int delete_time = (double)delete_count / total_operation_count * test_duration * 1000;
//   int lookup_time = (double)lookup_count / total_operation_count * test_duration * 1000;

//   // Print the time taken for each operation
//   std::cout << "Insert time: " << insert_time << " milliseconds" << std::endl;
//   std::cout << "Delete time: " << delete_time << " milliseconds" << std::endl;
//   std::cout << "Lookup time: " << lookup_time << " milliseconds" << std::endl;

//   // Write the values to the file
//   outfile << "Insert time: " << insert_time << " milliseconds" << std::endl;
//   outfile << "Delete time: " << delete_time << " milliseconds" << std::endl;
//   outfile << "Lookup time: " << lookup_time << " milliseconds" << std::endl;

//   // Close the file
//   outfile.close();
// }

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  if (argc > 1) {
    if (strcmp("true", argv[1]) == 0) {
      printf("===Table Validation Enabled===\n");
      validateTable = true;
    } else {
      printf("===Table Validation Disabled===\n");
      validateTable = false;
    }
  }

  return RUN_ALL_TESTS();
}