#include "../quotient_filter_graveyard_hashing/quotient_filter_graveyard_hashing.h"
#include "../quotient_filter/quotient_filter.h"
#include <bitset>
#include <iostream>
#include <chrono>
#include <random>
#include <fstream>
#include <thread>
#include <functional>
#include <limits>
// disables a warning for converting ints to uint64_t
#pragma warning( disable: 4838 )

const int DURATION = 60;
const double MAX_FULLNESS = 0.95;
const double FILL_STEP = 0.05;
const int FILTER_Q = 20;

const int MIN_VALUE = INT_MIN;
const int MAX_VALUE = INT_MAX;

const std::string RESULT_FOLDER = "results_revised/";

// allows both quotient filters to be used interchangeablly
class AbstractQuotientFilter
{
private:
    bool isGraveyardFilter;
    QuotientFilter* qf;
    QuotientFilterGraveyard* qfg;
public:
    int table_size;
    AbstractQuotientFilter(QuotientFilter* qf) {
        this->qf = qf;
        this->table_size = qf->table_size;
        this->isGraveyardFilter = false;
    }
    AbstractQuotientFilter(QuotientFilterGraveyard* qfg) {
        this->qfg = qfg;
        this->table_size = qfg->table_size;
        this->isGraveyardFilter = true;
    }
    void insertElement(int value) {
        if (this->isGraveyardFilter) {
            this->qfg->insertElement(value);
        } else {
            this->qf->insertElement(value);
        }
    }
    void deleteElement(int value) {
        if (this->isGraveyardFilter) {
            this->qfg->deleteElement(value);
        } else {
            this->qf->deleteElement(value);
        }
    }
    bool query(int value) {
        if (this->isGraveyardFilter) {
            return this->qfg->query(value);
        } else {
            return this->qf->query(value);
        }
    }
};

// To be used as the hash function for testing
int hash_fn(int x) {
    std::hash<int> hash_fn;
    return hash_fn(x);
}

// Function to generate a random number between a range
int generate_random_number(int min, int max) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(min, max);
  return dis(gen);
}

struct queryResults {
    int random_lookup_count;
    int successful_lookup_count;
};

// Inserts some subset of a given element array and returns time taken
std::chrono::microseconds insertionSubTest(AbstractQuotientFilter* qf, int numbers_to_insert[],
        int start_idx, int stop_idx) {

    auto insert_start_time = std::chrono::steady_clock::now();
    for (int i = start_idx; i < stop_idx; i++) {
        qf->insertElement(numbers_to_insert[i]);
    }
    auto insert_end_time = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(insert_end_time - insert_start_time);
}

// Deletes some subset of a given element array and returns time taken
std::chrono::microseconds deletionSubTest(AbstractQuotientFilter* qf, int numbers_to_insert[],
        int start_idx, int stop_idx) {

    auto delete_start_time = std::chrono::steady_clock::now();
    for (int i = start_idx; i < stop_idx; i++) {
        qf->deleteElement(numbers_to_insert[i]);
    }
    auto delete_end_time = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(delete_end_time - delete_start_time);
}

// Performs successful and random queries, returning how many were completed
queryResults querySubTest(AbstractQuotientFilter* qf, int numbers_to_insert[], int stop_idx) {

    auto query_duration = std::chrono::seconds(DURATION);

    // Perform random lookups
    printf("random lookups for %d seconds...", DURATION);
    int random_lookup_count = 0;
    auto r_lookup_end_time = std::chrono::high_resolution_clock::now() + query_duration;
    while (std::chrono::high_resolution_clock::now() < r_lookup_end_time) {
        int query_value = generate_random_number(MIN_VALUE, MAX_VALUE);
        qf->query(query_value);
        random_lookup_count++;
    }
    printf(" done\n");

    // Perform successful lookups
    printf("successful lookups for %d seconds...", DURATION);
    int successful_lookup_count = 0;
    auto s_lookup_end_time = std::chrono::high_resolution_clock::now() + query_duration;
    while (std::chrono::high_resolution_clock::now() < s_lookup_end_time) {
        int query_value = numbers_to_insert[generate_random_number(0, stop_idx)];
        qf->query(query_value);
        successful_lookup_count++;
    }
    printf(" done\n");

    queryResults results = {random_lookup_count, successful_lookup_count};
    return results;
}

void perfTestInsert(AbstractQuotientFilter* qf, std::string filename) {
    // Open output file
    std::ofstream outfile(filename + "_perfInsert.txt");

    // Generate numbers to insert 
    const int table_size = qf->table_size;
    int numbers_to_insert[table_size];
    for (int i = 0; i < table_size; i ++) {
        numbers_to_insert[i] = generate_random_number(MIN_VALUE, MAX_VALUE);
    }

    // Tracks the set of values to be inserted next
    double current_fullness = FILL_STEP;
    int start_idx = 0;
    int stop_idx = table_size * current_fullness;

    while (current_fullness <= MAX_FULLNESS) {
        // Insert elements
        printf("perfInsert: insert to %.2f fullness...", current_fullness);
        auto insert_time = insertionSubTest(qf, numbers_to_insert, start_idx, stop_idx).count();
        printf(" done\n");

        queryResults query_results = querySubTest(qf, numbers_to_insert, stop_idx);
        int random_lookup_count = query_results.random_lookup_count;
        int successful_lookup_count = query_results.successful_lookup_count;

        // Write Results
        outfile << "Current Fullness: " << current_fullness << ", ";
        outfile << "Number Inserted: " << (stop_idx - start_idx) << ", ";
        outfile << "Time Taken: " << insert_time << " micros\n";
        outfile << "Query duration: " << DURATION << " sec, ";
        outfile << "Random Queries: " << random_lookup_count << ", ";
        outfile << "Successful Queries: " << successful_lookup_count << "\n";
        outfile << "-------\n";

        // Increment for next loop
        current_fullness += FILL_STEP;
        start_idx = stop_idx;
        stop_idx = table_size * current_fullness;
    }

    outfile.close();
    printf("perfInsert: completed\n");
}

void perfTestDelete(AbstractQuotientFilter* qf, std::string filename) {
    // Open output file
    std::ofstream outfile(filename + "_perfDelete.txt");

    // Generate numbers to insert 
    const int table_size = qf->table_size;
    int numbers_to_insert[table_size];
    for (int i = 0; i < table_size; i ++) {
        numbers_to_insert[i] = generate_random_number(MIN_VALUE, MAX_VALUE);
    }

    // Tracks the set of values to be deleted next
    double current_fullness = MAX_FULLNESS;
    int stop_idx = table_size * current_fullness;
    int start_idx = table_size * (current_fullness - FILL_STEP);

    // Initially fills table to limit
    printf("perfDelete: initial insertions to %.2f fullness...", MAX_FULLNESS);
    for (int i = 0; i < stop_idx; i++) {
        qf->insertElement(numbers_to_insert[i]);
    }
    printf(" done\n");

    while (current_fullness >= FILL_STEP) {
        // Delete elements
        printf("perfDelete: delete from %.2f fullness...", current_fullness);
        auto delete_time = deletionSubTest(qf, numbers_to_insert, start_idx, stop_idx).count();
        printf(" done\n");

        queryResults query_results = querySubTest(qf, numbers_to_insert, stop_idx);
        int random_lookup_count = query_results.random_lookup_count;
        int successful_lookup_count = query_results.successful_lookup_count;

        // Write Results
        outfile << "Current Fullness: " << current_fullness << ", ";
        outfile << "Number Deleted: " << (stop_idx - start_idx) << ", ";
        outfile << "Time Taken: " << delete_time << " micros\n";
        outfile << "Query duration: " << DURATION << " sec, ";
        outfile << "Random Queries: " << random_lookup_count << ", ";
        outfile << "Successful Queries: " << successful_lookup_count << "\n";
        outfile << "-------\n";

        // Increment for next loop
        current_fullness -= FILL_STEP;
        stop_idx = start_idx;
        start_idx = table_size * (current_fullness - FILL_STEP);
        if (start_idx < 0) {start_idx = 0;}
    }

    outfile.close();
    printf("perfDelete: completed\n");
}

void perfTestMixed(AbstractQuotientFilter* qf, std::string filename) {
    // Open output file
    std::ofstream outfile(filename + "_perfMixed.txt");

    // Generate numbers to insert 
    const int table_size = qf->table_size;
    int numbers_to_insert[table_size];
    for (int i = 0; i < table_size; i ++) {
        numbers_to_insert[i] = generate_random_number(MIN_VALUE, MAX_VALUE);
    }

    // Tracks the set of values to be inserted and deleted next
    double current_fullness = FILL_STEP;
    int start_idx = 0;
    int mid_idx = table_size * current_fullness;
    int stop_idx = table_size * (current_fullness + FILL_STEP);

    while (current_fullness <= MAX_FULLNESS - FILL_STEP) {
        // Insert elements
        printf("perfMixed: insert to %.2f fullness...", current_fullness + FILL_STEP);
        auto insert_time = insertionSubTest(qf, numbers_to_insert, start_idx, stop_idx).count();
        printf(" done\n");

        // Mixed elements
        printf("perfMixed: delete to %.2f fullness...", current_fullness);
        auto delete_time = deletionSubTest(qf, numbers_to_insert, mid_idx, stop_idx).count();
        printf(" done\n");

        queryResults query_results = querySubTest(qf, numbers_to_insert, mid_idx);
        int random_lookup_count = query_results.random_lookup_count;
        int successful_lookup_count = query_results.successful_lookup_count;

        // Write Results
        outfile << "Current Fullness: " << current_fullness << ", ";
        outfile << "Number Inserted: " << (stop_idx - start_idx) << ", ";
        outfile << "Time Taken: " << insert_time << " micros; ";
        outfile << "Number Deleted: " << (stop_idx - mid_idx) << ", ";
        outfile << "Time Taken: " << delete_time << " micros\n";
        outfile << "Query duration: " << DURATION << " sec, ";
        outfile << "Random Queries: " << random_lookup_count << ", ";
        outfile << "Successful Queries: " << successful_lookup_count << "\n";
        outfile << "-------\n";

        // Increment for next loop
        current_fullness += FILL_STEP;
        start_idx = mid_idx;
        mid_idx = stop_idx;
        stop_idx = table_size * (current_fullness + FILL_STEP);
    }

    outfile.close();
    printf("perfMixed: completed\n");
}



int main(int argc, char **argv) {
    // normal filter
    std::string normal_filter_name = RESULT_FOLDER + "normal";
    perfTestInsert(new AbstractQuotientFilter(new QuotientFilter(FILTER_Q, &hash_fn)),
        normal_filter_name);
    perfTestDelete(new AbstractQuotientFilter(new QuotientFilter(FILTER_Q, &hash_fn)),
        normal_filter_name);
    perfTestMixed(new AbstractQuotientFilter(new QuotientFilter(FILTER_Q, &hash_fn)),
        normal_filter_name);

    // graveyard filters
    RedistributionPolicy policies[] = {no_redistribution, amortized_clean, between_runs, between_runs_insert, evenly_distribute};
    std::string filter_names[] = {"no_redist", "amortized_clean", "between_runs", "between_runs_insert", "evenly_dist"};

    for (int i = 0; i < 5; i ++) {
        RedistributionPolicy current_policy = policies[i];
        printf("\npolicy: %d\n", current_policy);
        std::string current_filter_name = RESULT_FOLDER + filter_names[i];
        perfTestInsert(new AbstractQuotientFilter(
            new QuotientFilterGraveyard(FILTER_Q, &hash_fn, current_policy)),
            current_filter_name);
        perfTestDelete(new AbstractQuotientFilter(
            new QuotientFilterGraveyard(FILTER_Q, &hash_fn, current_policy)),
            current_filter_name);
        perfTestMixed(new AbstractQuotientFilter(
            new QuotientFilterGraveyard(FILTER_Q, &hash_fn, current_policy)),
            current_filter_name);
    }
}