#include "../quotient_filter_graveyard_hashing/quotient_filter_graveyard_hashing.h"
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

const int DURATION = 20;
const double MAX_FULLNESS = 1;

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
void perfTestInsert(QuotientFilter* vqf, QuotientFilterGraveyard* qf, QuotientFilterGraveyard* qf2,QuotientFilterGraveyard* qf3, QuotientFilterGraveyard* qf4, 
                    std::string filename1, std::string filename2, std::string filename3, std::string filename4) {
  // Open output file
  std::ofstream outfile("perfInsert_"+filename1+ ".txt");
  std::ofstream outfile1("perfInsert_ordinary_16.txt");
  std::ofstream outfile2("perfInsert_"+filename2+ ".txt");
  std::ofstream outfile3("perfInsert_"+filename3+ ".txt");
  std::ofstream outfile4("perfInsert_"+filename4+ ".txt");

  //Get the list of numbers to insert into each filter
  const int filter_capacity = qf->table_size;
  int numbersToInsert[filter_capacity];
  const int fill_limit = filter_capacity * 0.05;

  // generates numbers to insert into filter
  for (int i = 0; i < filter_capacity; i++) {
    numbersToInsert[i] = generate_random_number(0, 10000);
  }

  float currentFullness = 0.00;
  int start;
  while (currentFullness <= MAX_FULLNESS) {
    // Calculate the number of elements to insert until the filter is 5% filled
    start = currentFullness*filter_capacity;
    std::cout << "Inserting at " << currentFullness << " fullness\n";


    // Insert elements until the filter is 5% filled
    auto start_inserts = std::chrono::steady_clock::now();
    for (int i = start; i < start+fill_limit; i++) {
      qf->insertElement(numbersToInsert[i]);
    }
    auto end_inserts = std::chrono::steady_clock::now();
    auto insert_time = std::chrono::duration_cast<std::chrono::microseconds>(end_inserts - start_inserts).count();
    outfile << "Current Fullness: " << currentFullness << ". Insertion " << fill_limit << " " << insert_time << " microseconds" << std::endl;
    std::cout << "Finished inserting in noredis\n";

    start_inserts = std::chrono::steady_clock::now();
    for (int i = start; i < start+fill_limit; i++) {
      vqf->insertElement(numbersToInsert[i]);
    }
    end_inserts = std::chrono::steady_clock::now();
    insert_time = std::chrono::duration_cast<std::chrono::microseconds>(end_inserts - start_inserts).count();
    outfile1 << "Current Fullness: " << currentFullness << ". Insertion " << fill_limit << " " << insert_time << " microseconds" << std::endl;
    std::cout << "Finished inserting in noredis\n";

    auto start_inserts2 = std::chrono::steady_clock::now();
    for (int i = 0; i < fill_limit; i++) {
      qf2->insertElement(numbersToInsert[i]);
    }
    auto end_inserts2 = std::chrono::steady_clock::now();
    auto insert_time2 = std::chrono::duration_cast<std::chrono::microseconds>(end_inserts2 - start_inserts2).count();
    outfile2 << "Current Fullness: " << currentFullness << ". Insertion " << fill_limit << " " << insert_time2 << " microseconds" << std::endl;
    std::cout << "Finished inserting in between-runs\n";

    auto start_inserts3 = std::chrono::steady_clock::now();
    for (int i = 0; i < fill_limit; i++) {
      qf3->insertElement(numbersToInsert[i]);
    }
    auto end_inserts3 = std::chrono::steady_clock::now();
    auto insert_time3 = std::chrono::duration_cast<std::chrono::microseconds>(end_inserts3 - start_inserts3).count();
    std::cout << "Finished inserting in between-runs-insert\n";
    outfile3 << "Current Fullness: " << currentFullness << ". Insertion " << fill_limit << " " << insert_time3 << " microseconds" << std::endl;

    auto start_inserts4 = std::chrono::steady_clock::now();
    for (int i = 0; i < fill_limit; i++) {
      qf4->insertElement(numbersToInsert[i]);
    }
    auto end_inserts4 = std::chrono::steady_clock::now();
    auto insert_time4 = std::chrono::duration_cast<std::chrono::microseconds>(end_inserts4 - start_inserts4).count();
    std::cout << "Finished inserting in evenly-distribute\n";
    outfile4 << "Current Fullness: " << currentFullness << ". Insertion " << fill_limit << " " << insert_time4 << " microseconds" << std::endl;

    //Perform Successful Lookups in each filter
    int numbersToLookup[600000];
    // generates numbers to insert into filter
    for (int i = 0; i < 600000; i++) {
      numbersToLookup[i] = generate_random_number(0, qf->table_size);
    }
    std::cout << "Starting successful lookups\n";
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 600000; i++) {
      qf->query(numbersToLookup[i]);;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto query_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    // Document throughput
    outfile <<  "600000 successful lookups in " << query_time << std::endl;

    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 600000; i++) {
      qf2->query(numbersToLookup[i]);;
    }
    end = std::chrono::high_resolution_clock::now();
    query_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    // Document throughput
    outfile2 <<  "600000 successful lookups in " << query_time << std::endl;

    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 600000; i++) {
      qf3->query(numbersToLookup[i]);;
    }
    end = std::chrono::high_resolution_clock::now();
    query_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    // Document throughput
    outfile3 <<  "600000 successful lookups in " << query_time << std::endl;

    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 600000; i++) {
      qf4->query(numbersToLookup[i]);;
    }
    end = std::chrono::high_resolution_clock::now();
    query_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    // Document throughput
    outfile4 <<  "600000 successful lookups in " << query_time << std::endl;

    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 600000; i++) {
      vqf->query(numbersToLookup[i]);;
    }
    end = std::chrono::high_resolution_clock::now();
    query_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    // Document throughput
    outfile1 <<  "600000 successful lookups in " << query_time << std::endl;
    std::cout << "Finished successful lookups\n";
    //Perform random lookups
    std::cout << "Performing random lookups\n";
    for (int i = 0; i < 600000; i++) {
      numbersToLookup[i] = numbersToInsert[generate_random_number(0, filter_capacity)];
    }

    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 600000; i++) {
      qf->query(numbersToLookup[i]);;
    }
    end = std::chrono::high_resolution_clock::now();
    query_time = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    // Document throughput
    outfile <<  "600000 random lookups in " << query_time << std::endl;

    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 600000; i++) {
      qf2->query(numbersToLookup[i]);;
    }
    end = std::chrono::high_resolution_clock::now();
    query_time = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    // Document throughput
    outfile2 << "600000 random lookups in " << query_time << std::endl;

    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 600000; i++) {
      qf3->query(numbersToLookup[i]);;
    }
    end = std::chrono::high_resolution_clock::now();
    query_time = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    // Document throughput
    outfile3 << "600000 random lookups in " << query_time << std::endl;

    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 600000; i++) {
      qf4->query(numbersToLookup[i]);;
    }
    end = std::chrono::high_resolution_clock::now();
    query_time = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    // Document throughput
    outfile4 << "600000 random lookups in " << query_time << std::endl;

    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 600000; i++) {
      vqf->query(numbersToLookup[i]);;
    }
    end = std::chrono::high_resolution_clock::now();
    query_time = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    // Document throughput
    outfile1 << "600000 random lookups in " << query_time << std::endl;

    //Increment the current Fullness
    currentFullness += 0.05;
  }
  
  // Close the file
  outfile.close();
  outfile1.close();
  outfile2.close();
  outfile3.close();
  outfile4.close();
}

// TEST_F(QuotientFilterTest, PerfDelete) {
void perfTestDelete(QuotientFilter* vqf, QuotientFilterGraveyard* qf, QuotientFilterGraveyard* qf2, QuotientFilterGraveyard* qf3,
                    QuotientFilterGraveyard* qf4, std::string filename, std::string filename2, std::string filename3,
                    std::string filename4) {
  
  // Open output files
  std::ofstream outfile1("perfDelete_ordinary.txt");
  std::ofstream outfile("perfDelete_"+filename+ ".txt");
  std::ofstream outfile2("perfDelete_"+filename2+ ".txt");
  std::ofstream outfile3("perfDelete_"+filename3+ ".txt");
  std::ofstream outfile4("perfDelete_"+filename4+ ".txt");

  //Get the list of numbers to insert into each filter
  const int filter_capacity = qf->table_size;
  int numbersToInsert[filter_capacity];
  // generates numbers to insert into filter
  for (int i = 0; i < filter_capacity; i++) {
    numbersToInsert[i] = generate_random_number(0, 10000);
  }


  // fill filter with values to delete!
  for (int i = 0; i < filter_capacity; i++) {
    qf->insertElement(numbersToInsert[i]);
    qf2->insertElement(numbersToInsert[i]);
    qf3->insertElement(numbersToInsert[i]);
    qf4->insertElement(numbersToInsert[i]);
    vqf->insertElement(numbersToInsert[i]);
  }

  float currentFullness = MAX_FULLNESS;
  while (currentFullness >= 0.05) {
    // Calculate the number of elements to insert until the filter is 5% filled
    std::cout << "Deleting starting at " << currentFullness << " fullness\n";
    int remainder_max = (1 << qf->r);

    // Insert elements until the filter is 5% filled
    int deletePosition = currentFullness * filter_capacity - 1; // how far into the array we're at
    int deleteMinPosition = deletePosition - 0.05 * filter_capacity;
    auto start_inserts = std::chrono::steady_clock::now();
    for (int i = deletePosition; i > deleteMinPosition; i--) {
      qf->deleteElement(numbersToInsert[i]);
    }
    auto end_inserts = std::chrono::steady_clock::now();
    auto delete_time = std::chrono::duration_cast<std::chrono::microseconds>(end_inserts - start_inserts).count();
    outfile << "Current Fullness: " << currentFullness << ". Deleted " << 0.05*filter_capacity << " in " << delete_time << " microseconds" << std::endl;

    start_inserts = std::chrono::steady_clock::now();
    for (int i = deletePosition; i > deleteMinPosition; i--) {
      qf2->deleteElement(numbersToInsert[i]);
    }
    end_inserts = std::chrono::steady_clock::now();
    delete_time = std::chrono::duration_cast<std::chrono::microseconds>(end_inserts - start_inserts).count();
    outfile2 << "Current Fullness: " << currentFullness << ". Deleted " << 0.05*filter_capacity << " in " << delete_time << " microseconds" << std::endl;

    start_inserts = std::chrono::steady_clock::now();
    for (int i = deletePosition; i > deleteMinPosition; i--) {
      qf3->deleteElement(numbersToInsert[i]);
    }
    end_inserts = std::chrono::steady_clock::now();
    delete_time = std::chrono::duration_cast<std::chrono::microseconds>(end_inserts - start_inserts).count();
    outfile3 << "Current Fullness: " << currentFullness << ". Deleted " << 0.05*filter_capacity << " in " << delete_time << " microseconds" << std::endl;
    
    start_inserts = std::chrono::steady_clock::now();
    for (int i = deletePosition; i > deleteMinPosition; i--) {
      qf4->deleteElement(numbersToInsert[i]);
    }
    end_inserts = std::chrono::steady_clock::now();
    delete_time = std::chrono::duration_cast<std::chrono::microseconds>(end_inserts - start_inserts).count();
    outfile4 << "Current Fullness: " << currentFullness << ". Deleted " << 0.05*filter_capacity << " in " << delete_time << " microseconds" << std::endl;

    start_inserts = std::chrono::steady_clock::now();
    for (int i = deletePosition; i > deleteMinPosition; i--) {
      vqf->deleteElement(numbersToInsert[i]);
    }
    end_inserts = std::chrono::steady_clock::now();
    delete_time = std::chrono::duration_cast<std::chrono::microseconds>(end_inserts - start_inserts).count();
    outfile1 << "Current Fullness: " << currentFullness << ". Deleted " << 0.05*filter_capacity << " in " << delete_time << " microseconds" << std::endl;

    //Perform Successful Lookups in each filter
    int numbersToLookup[600000];
    // generates numbers to insert into filter
    for (int i = 0; i < 600000; i++) {
      numbersToLookup[i] = generate_random_number(0, qf->table_size);
    }

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 600000; i++) {
      qf->query(numbersToLookup[i]);;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto query_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    // Document throughput
    outfile <<  "600000 successful lookups in " << query_time << std::endl;

    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 600000; i++) {
      qf2->query(numbersToLookup[i]);;
    }
    end = std::chrono::high_resolution_clock::now();
    query_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    // Document throughput
    outfile2 <<  "600000 successful lookups in " << query_time << std::endl;

    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 600000; i++) {
      qf3->query(numbersToLookup[i]);;
    }
    end = std::chrono::high_resolution_clock::now();
    query_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    // Document throughput
    outfile3 <<  "600000 successful lookups in " << query_time << std::endl;

    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 600000; i++) {
      qf4->query(numbersToLookup[i]);;
    }
    end = std::chrono::high_resolution_clock::now();
    query_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    // Document throughput
    outfile4 <<  "600000 successful lookups in " << query_time << std::endl;

    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 600000; i++) {
      vqf->query(numbersToLookup[i]);;
    }
    end = std::chrono::high_resolution_clock::now();
    query_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    // Document throughput
    outfile1 <<  "600000 successful lookups in " << query_time << std::endl;

    //Perform random lookups
    for (int i = 0; i < 600000; i++) {
      numbersToLookup[i] = numbersToInsert[generate_random_number(0, filter_capacity)];
    }

    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 600000; i++) {
      qf->query(numbersToLookup[i]);;
    }
    end = std::chrono::high_resolution_clock::now();
    query_time = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    // Document throughput
    outfile <<  "600000 random lookups in " << query_time << std::endl;

    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 600000; i++) {
      qf2->query(numbersToLookup[i]);;
    }
    end = std::chrono::high_resolution_clock::now();
    query_time = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    // Document throughput
    outfile2 << "600000 random lookups in " << query_time << std::endl;

    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 600000; i++) {
      qf3->query(numbersToLookup[i]);;
    }
    end = std::chrono::high_resolution_clock::now();
    query_time = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    // Document throughput
    outfile3 << "600000 random lookups in " << query_time << std::endl;

    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 600000; i++) {
      qf4->query(numbersToLookup[i]);;
    }
    end = std::chrono::high_resolution_clock::now();
    query_time = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    // Document throughput
    outfile4 << "600000 random lookups in " << query_time << std::endl;

    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 600000; i++) {
      vqf->query(numbersToLookup[i]);;
    }
    end = std::chrono::high_resolution_clock::now();
    query_time = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    // Document throughput
    outfile1 << "600000 random lookups in " << query_time << std::endl;

    //Increment the current Fullness
    currentFullness -= 0.05;
  }
  
  // Close the file
  outfile.close();
  outfile1.close();
  outfile2.close();
  outfile3.close();
  outfile4.close();
}

// TEST_F(QuotientFilterTest, PerfDelete) {
void perfTestDelete(QuotientFilterGraveyard* qf, std::string filename) {
  const int filter_capacity = qf->table_size;
  const int fill_limit = filter_capacity * MAX_FULLNESS;
  int numbersToInsert[fill_limit];
  // generates numbers to insert into filter
  for (int i = 0; i < fill_limit; i++) {
    numbersToInsert[i] = generate_random_number(0, 10000);
  }

  // fill filter with values to delete!
  for (int i = 0; i < fill_limit; i++) {
    if (i==0) {
      std::cout << "First element we are inserting is " << numbersToInsert[i] << "\n";
    }
    qf->insertElement(numbersToInsert[i]);
  }
  std::cout << "Finished Inserting " << fill_limit << "\n";


  // Open output file
  std::ofstream outfile("perfDelete_"+filename+ ".txt");

  float currentFullness = MAX_FULLNESS;
  while (currentFullness >= 0.05) {
    // Calculate the number of elements to insert until the filter is 5% filled
    std::cout << "Deleting starting at " << currentFullness << " fullness\n";
    const int filter_capacity = qf->table_size;
    int remainder_max = (1 << qf->r);

    // Insert elements until the filter is 5% filled
    int deletePosition = currentFullness * filter_capacity - 1; // how far into the array we're at
    int deleteMinPosition = deletePosition - 0.05 * filter_capacity;
    auto start_inserts = std::chrono::steady_clock::now();
    for (int i = deletePosition; i > deleteMinPosition; i--) {
      std::cout << "Deleting number at position " << i << "\n";
      qf->deleteElement(numbersToInsert[i]);
      std::cout << "Deleted number at position " << i << "\n";
    }
    auto end_inserts = std::chrono::steady_clock::now();
    auto insert_time = std::chrono::duration_cast<std::chrono::microseconds>(end_inserts - start_inserts).count();
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
void perfTestMixed(QuotientFilterGraveyard* qf, std::string filename) {
  // Open output file
  std::ofstream outfile("perfMixed_"+filename+ ".txt");

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
    auto insert_time = std::chrono::duration_cast<std::chrono::microseconds>(end_inserts - start_inserts).count();
    outfile << "Current Fullness: " << currentFullness << ". Insertion " << fill_limit << " " << insert_time << " microseconds" << std::endl;


    // Delete elements until the filter is 5% filled
    auto start_deletes = std::chrono::steady_clock::now();
    for (int i = 0; i < delete_limit; i++) {
      qf->deleteElement(numbersToInsert[fill_limit - i - 1]);
    }

    auto end_deletes = std::chrono::steady_clock::now();
    auto delete_time = std::chrono::duration_cast<std::chrono::microseconds>(end_deletes - start_deletes).count();
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

void perfTestMixed(QuotientFilter* vqf, QuotientFilterGraveyard* qf, QuotientFilterGraveyard* qf2, QuotientFilterGraveyard* qf3,
                    QuotientFilterGraveyard* qf4, std::string filename, std::string filename2, std::string filename3,
                    std::string filename4) {
  // Open output file
  std::ofstream outfile1("perfMixed_ordinary.txt");
  std::ofstream outfile("perfMixed_"+filename+ ".txt");
  std::ofstream outfile2("perfMixed_"+filename2+ ".txt");
  std::ofstream outfile3("perfMixed_"+filename3+ ".txt");
  std::ofstream outfile4("perfMixed_"+filename4+ ".txt");
  const int filter_capacity = qf->table_size;
  const int fill_limit = filter_capacity * 0.1;
  const int delete_limit = filter_capacity * 0.05;
  int numbersToInsert[fill_limit*2];
  int numbersToDelete[fill_limit];
  int numbersToLookup[600000];

  float currentFullness = 0.05;
  while (currentFullness <= MAX_FULLNESS) {
    // Calculate the number of elements to insert until the filter is 5% filled

    // generates numbers to insert into filter
    for (int i = 0; i < fill_limit*2; i++) {
      numbersToInsert[i] = generate_random_number(0, 10000);
    }

    // Insert elements into filters until they are 5% filled
    auto start_inserts = std::chrono::steady_clock::now();
    for (int i = 0; i < fill_limit*2; i++) {
      qf->insertElement(numbersToInsert[i]);
    }
    auto end_inserts = std::chrono::steady_clock::now();
    auto insert_time = std::chrono::duration_cast<std::chrono::microseconds>(end_inserts - start_inserts).count();
    outfile << "Current Fullness: " << currentFullness << ". Insertion " << fill_limit << " " << insert_time << " microseconds" << std::endl;

    start_inserts = std::chrono::steady_clock::now();
    for (int i = 0; i < fill_limit; i++) {
      vqf->insertElement(numbersToInsert[i]);
    }
    end_inserts = std::chrono::steady_clock::now();
    insert_time = std::chrono::duration_cast<std::chrono::microseconds>(end_inserts - start_inserts).count();
    outfile1 << "Current Fullness: " << currentFullness << ". Insertion " << fill_limit << " " << insert_time << " microseconds" << std::endl;

    start_inserts = std::chrono::steady_clock::now();
    for (int i = 0; i < fill_limit; i++) {
      qf2->insertElement(numbersToInsert[i]);
    }
    end_inserts = std::chrono::steady_clock::now();
    insert_time = std::chrono::duration_cast<std::chrono::microseconds>(end_inserts - start_inserts).count();
    outfile2 << "Current Fullness: " << currentFullness << ". Insertion " << fill_limit << " " << insert_time << " microseconds" << std::endl;

    start_inserts = std::chrono::steady_clock::now();
    for (int i = 0; i < fill_limit; i++) {
      qf3->insertElement(numbersToInsert[i]);
    }
    end_inserts = std::chrono::steady_clock::now();
    insert_time = std::chrono::duration_cast<std::chrono::microseconds>(end_inserts - start_inserts).count();
    outfile3 << "Current Fullness: " << currentFullness << ". Insertion " << fill_limit << " " << insert_time << " microseconds" << std::endl;

    start_inserts = std::chrono::steady_clock::now();
    for (int i = 0; i < fill_limit; i++) {
      qf4->insertElement(numbersToInsert[i]);
    }
    end_inserts = std::chrono::steady_clock::now();
    insert_time = std::chrono::duration_cast<std::chrono::microseconds>(end_inserts - start_inserts).count();
    outfile4 << "Current Fullness: " << currentFullness << ". Insertion " << fill_limit << " " << insert_time << " microseconds" << std::endl;

    // generates numbers to delete from filter

    for (int i = 0; i < fill_limit; i++) {
      numbersToDelete[i] = numbersToInsert[generate_random_number(0,2*fill_limit)];
    }

    // Delete elements until the filter is 5% filled
    auto start_deletes = std::chrono::steady_clock::now();
    for (int i = 0; i < delete_limit; i++) {
      qf->deleteElement(numbersToInsert[fill_limit - i - 1]);
    }
    auto end_deletes = std::chrono::steady_clock::now();
    auto delete_time = std::chrono::duration_cast<std::chrono::microseconds>(end_deletes - start_deletes).count();
    outfile << "Current Fullness: " << currentFullness << ". Deletion " << delete_limit << " " << delete_time << " microseconds" << std::endl;

    start_deletes = std::chrono::steady_clock::now();
    for (int i = 0; i < delete_limit; i++) {
      vqf->deleteElement(numbersToInsert[fill_limit - i - 1]);
    }
    end_deletes = std::chrono::steady_clock::now();
    delete_time = std::chrono::duration_cast<std::chrono::microseconds>(end_deletes - start_deletes).count();
    outfile1 << "Current Fullness: " << currentFullness << ". Deletion " << delete_limit << " " << delete_time << " microseconds" << std::endl;

    start_deletes = std::chrono::steady_clock::now();
    for (int i = 0; i < delete_limit; i++) {
      qf2->deleteElement(numbersToInsert[fill_limit - i - 1]);
    }
    end_deletes = std::chrono::steady_clock::now();
    delete_time = std::chrono::duration_cast<std::chrono::microseconds>(end_deletes - start_deletes).count();
    outfile2 << "Current Fullness: " << currentFullness << ". Deletion " << delete_limit << " " << delete_time << " microseconds" << std::endl;

    start_deletes = std::chrono::steady_clock::now();
    for (int i = 0; i < delete_limit; i++) {
      qf3->deleteElement(numbersToInsert[fill_limit - i - 1]);
    }
    end_deletes = std::chrono::steady_clock::now();
    delete_time = std::chrono::duration_cast<std::chrono::microseconds>(end_deletes - start_deletes).count();
    outfile3 << "Current Fullness: " << currentFullness << ". Deletion " << delete_limit << " " << delete_time << " microseconds" << std::endl;

    start_deletes = std::chrono::steady_clock::now();
    for (int i = 0; i < delete_limit; i++) {
      qf4->deleteElement(numbersToInsert[fill_limit - i - 1]);
    }
    end_deletes = std::chrono::steady_clock::now();
    delete_time = std::chrono::duration_cast<std::chrono::microseconds>(end_deletes - start_deletes).count();
    outfile4 << "Current Fullness: " << currentFullness << ". Deletion " << delete_limit << " " << delete_time << " microseconds" << std::endl;


    //Get elements to lookup
    for (int i=0; i<600000; i++) {
      numbersToLookup[i] = qfv(15,17);
    }
    // perform queries for 60%
    auto start = std::chrono::high_resolution_clock::now();
    for (int i=0; i<600000; i++) {
        qf->query(numbersToLookup[i]);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto query_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    // Write the random lookup values to file
    outfile <<  "600000 successful lookups in " << query_time << std::endl;

    start = std::chrono::high_resolution_clock::now();
    for (int i=0; i<600000; i++) {
        vqf->query(numbersToLookup[i]);
    }
    end = std::chrono::high_resolution_clock::now();
    query_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    // Write the random lookup values to file
    outfile1 <<  "600000 successful lookups in " << query_time << std::endl;

    start = std::chrono::high_resolution_clock::now();
    for (int i=0; i<600000; i++) {
        qf2->query(numbersToLookup[i]);
    }
    end = std::chrono::high_resolution_clock::now();
    query_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    // Write the random lookup values to file
    outfile2 <<  "600000 successful lookups in " << query_time << std::endl;
  
    start = std::chrono::high_resolution_clock::now();
    for (int i=0; i<600000; i++) {
        qf3->query(numbersToLookup[i]);
    }
    end = std::chrono::high_resolution_clock::now();
    query_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    // Write the random lookup values to file
    outfile3 <<  "600000 successful lookups in " << query_time << std::endl;

    start = std::chrono::high_resolution_clock::now();
    for (int i=0; i<600000; i++) {
        qf4->query(numbersToLookup[i]);
    }
    end = std::chrono::high_resolution_clock::now();
    query_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    // Write the random lookup values to file
    outfile4 <<  "600000 successful lookups in " << query_time << std::endl;
    currentFullness += 0.05;
  }
  

  // Close the file
  outfile.close();
}

int main(int argc, char **argv) {
  QuotientFilter vqf = QuotientFilter(15, &hash_fn);
  QuotientFilterGraveyard qf = QuotientFilterGraveyard(16, &hash_fn, no_redistribution);
  QuotientFilterGraveyard qf2 = QuotientFilterGraveyard(16, &hash_fn, between_runs);
  QuotientFilterGraveyard qf3 = QuotientFilterGraveyard(16, &hash_fn, between_runs_insert);
  QuotientFilterGraveyard qf4 = QuotientFilterGraveyard(16, &hash_fn, evenly_distribute);
  std::string filenames[] = {"no_reditsribution_16", "between_runs_16", "between_runs_insert_16", "evenly_distribute_16"};
  perfTestInsert(&vqf, &qf, &qf2, &qf3, &qf4, filenames[0], filenames[1], filenames[2],filenames[3]);
  // vqf = QuotientFilter(10, &hash_fn);
  // qf = QuotientFilterGraveyard(10, &hash_fn, no_redistribution);
  // qf2 = QuotientFilterGraveyard(10, &hash_fn, between_runs);
  // qf3 = QuotientFilterGraveyard(10, &hash_fn, between_runs_insert);
  // qf4 = QuotientFilterGraveyard(10, &hash_fn, evenly_distribute);
  // perfTestDelete(&vqf, &qf, &qf2, &qf3, &qf4, filenames[0], filenames[1], filenames[2],filenames[3]);
}
