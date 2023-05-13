#include <gtest/gtest.h>
#include "../quotient_filter_graveyard_hashing/quotient_filter_graveyard_hashing.h"
#include <bitset>
// disables a warning for converting ints to uint64_t
#pragma warning( disable: 4838 )

// To be used as the hash function for testing
int identity(int x) {
    return x;
}

bool validateTable;

// Fixture classes
class RedistributionTestBetweenRuns : public ::testing::Test {
  protected:
    QuotientFilterGraveyard* qf;

    void SetUp() override {
      qf = new QuotientFilterGraveyard(4, &identity, between_runs);
    }

    // void TearDown() override {}
};

class RedistributionTestBetweenRunsInsert : public ::testing::Test {
  protected:
    QuotientFilterGraveyard* qf;

    void SetUp() override {
      qf = new QuotientFilterGraveyard(4, &identity, between_runs_insert);
    }

    // void TearDown() override {}
};

class RedistributionTestEvenlyDistribute : public ::testing::Test {
  protected:
    QuotientFilterGraveyard* qf;

    void SetUp() override {
      qf = new QuotientFilterGraveyard(4, &identity, evenly_distribute);
    }

    // void TearDown() override {}
};

// Builds a value to insert into the filter, based on q and r
// q should be less than 16
int qfv(int q, int r) {
  return ((q & 15) << 28) + r;
}

// builds an expected filter value, based on a predecessor and successor
uint64_t psv(uint64_t p, uint64_t s) {
  return (p << 32) + s;
}

// Checks to see that all buckets are empty, except for the given exceptions
// exceptions should be sorted
void assert_empty_buckets(QuotientFilterGraveyard* qf, int exceptionCount, int exceptions[]) {
  int ce = 0;
  for (int bucket = 0; bucket < 16; bucket++) {
    if (ce < exceptionCount && exceptions[ce] == bucket) {
      ce++;
      continue;
    }

    EXPECT_FALSE(qf->query(qfv(bucket, 0)));
    if (validateTable) {
      QuotientFilterElement elt = qf->table[bucket];
      EXPECT_EQ(elt.is_occupied, 0);
      EXPECT_EQ(elt.is_shifted, 0);
      EXPECT_EQ(elt.is_continuation, 0);
      EXPECT_EQ(elt.isTombstone, 0);
      // EXPECT_EQ(elt.value, 0);
    }
  }
}

// test for a location's metadata bits
typedef struct{
  int slot;
  bool is_occupied;
  bool is_shifted;
  bool is_continuation;
  bool isTombstone;
  uint64_t value;
} Mdt;

typedef struct{
  int slot;
  bool is_occupied;
  bool is_shifted;
  bool is_continuation;
  bool isTombstone;
  bool isEndOfCluster;
  uint64_t value;
} Mdt2;

void check_slots(QuotientFilterGraveyard* qf, int test_count, Mdt metadata_tests[]) {
  if (!validateTable) {
    return;
  }

  for (int i = 0; i < test_count; i++) {
    Mdt test = metadata_tests[i];
    QuotientFilterElement elt = qf->table[test.slot];

    EXPECT_EQ(elt.is_occupied, test.is_occupied);
    EXPECT_EQ(elt.is_shifted, test.is_shifted);
    EXPECT_EQ(elt.is_continuation, test.is_continuation);
    EXPECT_EQ(elt.isTombstone, test.isTombstone);
    EXPECT_EQ(elt.value, test.value);
  }
}

void check_slots(QuotientFilterGraveyard* qf, int test_count, Mdt2 metadata_tests[]) {
  if (!validateTable) {
    return;
  }

  for (int i = 0; i < test_count; i++) {
    Mdt2 test = metadata_tests[i];
    QuotientFilterElement elt = qf->table[test.slot];

    EXPECT_EQ(elt.is_occupied, test.is_occupied);
    EXPECT_EQ(elt.is_shifted, test.is_shifted);
    EXPECT_EQ(elt.is_continuation, test.is_continuation);
    EXPECT_EQ(elt.isTombstone, test.isTombstone);
    if (test.isEndOfCluster) {
      EXPECT_TRUE(elt.isEndOfCluster);
    } else {
      EXPECT_EQ(elt.value, test.value);
    }
  }
}

// debug printing
void scan_table(QuotientFilterGraveyard* qf) {
  for (int b = 0; b < 16; b++) {
    QuotientFilterElement elt = qf->table[b];
    std::cerr << "[table debug]" << b << ": " <<
      elt.is_occupied << elt.is_shifted << elt.is_continuation <<
      "[" << elt.isTombstone << "], " << elt.value << "\n";
  }
}


//// Note: base rebuild window size = 0.4 * 16 = (int) 6.4 = 6

///////// Between Runs Policy

// Checks that a rebuild has no effect if there have been no deletions
TEST_F(RedistributionTestBetweenRuns, InsertOnly) {
  int remainders[] = {12, 323, 5942, 102, 3, 6, 394, 20, 100, 1131};
  int b[10];
  Mdt slot_tests[10];

  for (int i = 0; i < 10; i ++) {
    qf->insertElement(qfv(i, remainders[i]));
    b[i] = i;
    slot_tests[i] = {b[i], true, false, false, false, (uint64_t)remainders[i]};
  }

  assert_empty_buckets(qf, 10, b);
  check_slots(qf, 10, slot_tests);
  EXPECT_EQ(qf->size, 10);
}

// Checks that a rebuild clears a table containing only tombstones
TEST_F(RedistributionTestBetweenRuns, TombstonesOnly) {
  int starting_bucket = 6;
  int remainders[] = {111, 222, 333};

  for (int i = 0; i < 3; i ++) {
    qf->insertElement(qfv((starting_bucket + i) % 16, remainders[i]));
  }
  for (int i = 0; i < 3; i ++) {
    qf->deleteElement(qfv((starting_bucket + i) % 16, remainders[i]));
  }
  // Rebuild expected after 6 operations

  assert_empty_buckets(qf, 0, NULL);
  EXPECT_EQ(qf->size, 0);
}

// Checks that single tombstones between clusters are unaffected
TEST_F(RedistributionTestBetweenRuns, ClusterGap) {
  int starting_bucket = 15;
  int remainders[] = {111, 222, 333, 444, 555};
  int b[5];

  for (int i = 0; i < 3; i ++) {
    qf->insertElement(qfv(starting_bucket, remainders[i]));
    b[i] = (starting_bucket + i) % 16;
  }
  qf->deleteElement(qfv(starting_bucket, remainders[2]));
  for (int i = 3; i < 5; i ++) {
    qf->insertElement(qfv((starting_bucket + 3) % 16, remainders[i]));
    b[i] = (starting_bucket + i) % 16;
  }
  // Rebuild expected after 6 operations

  Mdt slot_tests[] = {{b[0], true, false, false, false, (uint64_t)remainders[0]},
                       {b[1], false, true, true, false, (uint64_t)remainders[1]},
                       {b[2], false, true, true, true, psv(b[0], b[3])},
                       {b[3], true, false, false, false, (uint64_t)remainders[3]},
                       {b[4], false, true, true, false, (uint64_t)remainders[4]}};
  check_slots(qf, 5, slot_tests);
  EXPECT_EQ(qf->size, 4);
}

// Checks that multiple tombstones between clusters are reduced to single tombstones
TEST_F(RedistributionTestBetweenRuns, ClusterCleaned) {
  int starting_bucket = 2;
  int remainders[] = {111, 222, 333, 444};
  int b[4];

  for (int i = 0; i < 3; i ++) {
    qf->insertElement(qfv(starting_bucket, remainders[i]));
    b[i] = (starting_bucket + i) % 16;
  }
  qf->insertElement(qfv((starting_bucket + 3) % 16, remainders[3]));
  b[3] = (starting_bucket + 3) % 16;

  qf->deleteElement(qfv(starting_bucket, remainders[1]));
  qf->deleteElement(qfv(starting_bucket, remainders[2]));
  // Rebuild expected after 6 operations

  int expected_buckets[] = {b[0], b[1], b[3]};
  assert_empty_buckets(qf, 3, expected_buckets);
  Mdt slot_tests[] = {{b[0], true, false, false, false, (uint64_t)remainders[0]},
                       {b[1], false, true, true, true, psv(b[0], b[0])},
                       {b[3], true, false, false, false, (uint64_t)remainders[3]}};
  check_slots(qf, 3, slot_tests);
  EXPECT_EQ(qf->size, 2);
}

// Checks that single tombstones between runs are unaffected
TEST_F(RedistributionTestBetweenRuns, RunGap) {
  int starting_bucket = 6;
  int remainders[] = {111, 222, 333, 444, 555};
  int b[5];

  for (int i = 0; i < 3; i ++) {
    qf->insertElement(qfv(starting_bucket, remainders[i]));
    b[i] = (starting_bucket + i) % 16;
  }
  for (int i = 3; i < 5; i ++) {
    qf->insertElement(qfv((starting_bucket + 1) % 16, remainders[i]));
    b[i] = (starting_bucket + i) % 16;
  }
  qf->deleteElement(qfv(starting_bucket, remainders[2]));
  // Rebuild expected after 6 operations

  Mdt slot_tests[] = {{b[0], true, false, false, false, (uint64_t)remainders[0]},
                       {b[1], true, true, true, false, (uint64_t)remainders[1]},
                       {b[2], false, true, true, true, psv(b[0], b[1])},
                       {b[3], false, true, false, false, (uint64_t)remainders[3]},
                       {b[4], false, true, true, false, (uint64_t)remainders[4]}};
  check_slots(qf, 5, slot_tests);
  EXPECT_EQ(qf->size, 4);
}

// Checks that multiple tombstones between runs are reduced to single tombstones
TEST_F(RedistributionTestBetweenRuns, RunCleaned) {
  int starting_bucket = 13;
  int remainders[] = {111, 222, 333, 444};
  int b[3];

  for (int i = 0; i < 3; i ++) {
    qf->insertElement(qfv(starting_bucket, remainders[i]));
    b[i] = (starting_bucket + i) % 16;
  }
  qf->insertElement(qfv((starting_bucket + 1) % 16, remainders[3]));

  qf->deleteElement(qfv(starting_bucket, remainders[1]));
  qf->deleteElement(qfv(starting_bucket, remainders[2]));
  // Rebuild expected after 6 operations

  assert_empty_buckets(qf, 3, b);
  Mdt slot_tests[] = {{b[0], true, false, false, false, (uint64_t)remainders[0]},
                       {b[1], true, true, true, true, psv(b[0], b[1])},
                       {b[2], false, true, false, false, (uint64_t)remainders[3]}};
  check_slots(qf, 3, slot_tests);
  EXPECT_EQ(qf->size, 2);
}

// Checks that multiple tombstones between runs can be passed to later runs
TEST_F(RedistributionTestBetweenRuns, PassTombstones) {
  int starting_bucket = 4;
  int remainders[] = {111, 222, 333, 444, 555, 666, 777, 888};
  int b[8];

  for (int i = 0; i < 4; i ++) {
    qf->insertElement(qfv(starting_bucket, remainders[i]));
    b[i] = (starting_bucket + i) % 16;
  }
  for (int i = 4; i < 6; i ++) {
    qf->insertElement(qfv((starting_bucket + 1) % 16, remainders[i]));
    b[i] = (starting_bucket + i) % 16;
  }
  // First rebuild expected after 6 operations
  // load factor = 3/8 -> x = 1/(5/8) = 8/5 = 1.6 -> window size = 16/(4 * x) = 16/6.4 = 2.5
  // -> rebuild interval 2

  for (int i = 6; i < 8; i ++) {
    qf->insertElement(qfv((starting_bucket + 2) % 16, remainders[i]));
    b[i] = (starting_bucket + i) % 16;
  }

  // load factor = 1/2 -> x = 1/(1/2) = 2 -> window size = 16/(4 * 2) = 2
  // -> rebuild interval 2
  qf->deleteElement(qfv(starting_bucket, remainders[1]));
  qf->deleteElement(qfv(starting_bucket, remainders[2]));

  assert_empty_buckets(qf, 8, b);
  Mdt slot_tests[] = {{b[0], true, false, false, false, (uint64_t)remainders[0]},
                       {b[1], true, true, true, false, (uint64_t)remainders[3]},
                       {b[2], true, true, true, true, psv(b[0], b[1])},
                       {b[3], false, true, false, false, (uint64_t)remainders[4]},
                       {b[4], false, true, true, false, (uint64_t)remainders[5]},
                       {b[5], false, true, true, true, psv(b[1], b[2])},
                       {b[6], false, true, false, false, (uint64_t)remainders[6]},
                       {b[7], false, true, true, false, (uint64_t)remainders[7]}};
  check_slots(qf, 10, slot_tests);
  EXPECT_EQ(qf->size, 6);
}



///////// Between Runs Insert Policy

// Checks that a rebuild leaves a single run with one tombstone
TEST_F(RedistributionTestBetweenRunsInsert, InsertOnlySingle) {
  int starting_bucket = 4;
  int remainders[] = {987, 65, 4, 321, 9, 8, 765, 43};
  int b[9];

  for (int i = 0; i < 6; i ++) {
    b[i] = (starting_bucket + i) % 16;
    qf->insertElement(qfv(starting_bucket, remainders[i]));
  }
  b[6] = ((starting_bucket + 6) % 16);
  // Rebuild expected after 6 operations; 1 tombstone inserted

  assert_empty_buckets(qf, 7, b);
  Mdt slot_tests1[] = {{b[0], true, false, false, false, (uint64_t)remainders[0]},
                      {b[1], false, true, true, false, (uint64_t)remainders[1]},
                      {b[2], false, true, true, false, (uint64_t)remainders[2]},
                      {b[3], false, true, true, false, (uint64_t)remainders[3]},
                      {b[4], false, true, true, false, (uint64_t)remainders[4]},
                      {b[5], false, true, true, false, (uint64_t)remainders[5]},
                      {b[6], false, true, true, true, psv(b[0], b[0])}};
  check_slots(qf, 7, slot_tests1);
  EXPECT_EQ(qf->size, 6);

  // load factor = 7/16 -> x = 1/(9/16) = 16/9 -> window size = 16/(4 * x) = 16/4 * 9/16 = 9/4 = 2.25
  // -> rebuild interval 2
  for (int i = 6; i < 8; i ++) {
    b[i] = (starting_bucket + 1 + i) % 16;
    qf->insertElement(qfv(starting_bucket, remainders[i]));
  }

  assert_empty_buckets(qf, 9, b);
  Mdt slot_tests2[] = {{b[0], true, false, false, false, (uint64_t)remainders[0]},
                      {b[1], false, true, true, false, (uint64_t)remainders[1]},
                      {b[2], false, true, true, false, (uint64_t)remainders[2]},
                      {b[3], false, true, true, false, (uint64_t)remainders[3]},
                      {b[4], false, true, true, false, (uint64_t)remainders[4]},
                      {b[5], false, true, true, false, (uint64_t)remainders[5]},
                      {b[6], false, true, true, false, (uint64_t)remainders[6]},
                      {b[7], false, true, true, false, (uint64_t)remainders[7]},
                      {b[8], false, true, true, true, psv(b[0], b[0])}};
  check_slots(qf, 9, slot_tests2);
  EXPECT_EQ(qf->size, 8);
}

// Checks that a rebuild leaves separate runs with a tombstone
TEST_F(RedistributionTestBetweenRunsInsert, InsertOnlySeparate) {
  int starting_bucket = 2;
  int remainders[] = {12, 323, 5942, 102, 3, 6};
  int b[12];

  for (int i = 0; i < 6; i ++) {
    b[i] = (starting_bucket + i) % 16;
    qf->insertElement(qfv(b[i], remainders[i]));
    b[i + 6] = (b[i] + 6) % 16;
  }
  // Rebuild expected after 6 operations

  assert_empty_buckets(qf, 12, b);
  Mdt slot_tests[] = {{b[0], true, false, false, false, (uint64_t)remainders[0]},
                      {b[1], true, true, true, true, psv(b[0], b[1])},
                      {b[2], true, true, false, false, (uint64_t)remainders[1]},
                      {b[3], true, true, true, true, psv(b[1], b[2])},
                      {b[4], true, true, false, false, (uint64_t)remainders[2]},
                      {b[5], true, true, true, true, psv(b[2], b[3])},
                      {b[6], false, true, false, false, (uint64_t)remainders[3]},
                      {b[7], false, true, true, true, psv(b[3], b[4])},
                      {b[8], false, true, false, false, (uint64_t)remainders[4]},
                      {b[9], false, true, true, true, psv(b[4], b[5])},
                      {b[10], false, true, false, false, (uint64_t)remainders[5]},
                      {b[11], false, true, true, true, psv(b[5], b[5])}};
  check_slots(qf, 12, slot_tests);
  EXPECT_EQ(qf->size, 6);
}

// Checks that a rebuild clears a table containing only tombstones
TEST_F(RedistributionTestBetweenRunsInsert, TombstonesOnly) {
  int starting_bucket = 6;
  int remainders[] = {111, 222, 333};

  for (int i = 0; i < 3; i ++) {
    qf->insertElement(qfv((starting_bucket + i) % 16, remainders[i]));
  }
  for (int i = 0; i < 3; i ++) {
    qf->deleteElement(qfv((starting_bucket + i) % 16, remainders[i]));
  }
  // Rebuild expected after 6 operations

  assert_empty_buckets(qf, 0, NULL);
  EXPECT_EQ(qf->size, 0);
}

// Checks that single tombstones between clusters are unaffected
TEST_F(RedistributionTestBetweenRunsInsert, ClusterGap) {
  int starting_bucket = 15;
  int remainders[] = {111, 222, 333, 444, 555};
  int b[6];

  for (int i = 0; i < 3; i ++) {
    qf->insertElement(qfv(starting_bucket, remainders[i]));
    b[i] = (starting_bucket + i) % 16;
  }
  qf->deleteElement(qfv(starting_bucket, remainders[2]));
  for (int i = 3; i < 5; i ++) {
    qf->insertElement(qfv((starting_bucket + 3) % 16, remainders[i]));
    b[i] = (starting_bucket + i) % 16;
  }
  b[5] = (starting_bucket + 5) % 16;
  // Rebuild expected after 6 operations

  Mdt slot_tests[] = {{b[0], true, false, false, false, (uint64_t)remainders[0]},
                       {b[1], false, true, true, false, (uint64_t)remainders[1]},
                       {b[2], false, true, true, true, psv(b[0], b[3])},
                       {b[3], true, false, false, false, (uint64_t)remainders[3]},
                       {b[4], false, true, true, false, (uint64_t)remainders[4]},
                       {b[5], false, true, true, true, psv(b[3], b[3])}};
  check_slots(qf, 6, slot_tests);
  EXPECT_EQ(qf->size, 4);
}

// Checks that multiple tombstones between clusters are reduced to single tombstones
TEST_F(RedistributionTestBetweenRunsInsert, ClusterCleaned) {
  int starting_bucket = 2;
  int remainders[] = {111, 222, 333, 444};
  int b[5];

  for (int i = 0; i < 3; i ++) {
    qf->insertElement(qfv(starting_bucket, remainders[i]));
    b[i] = (starting_bucket + i) % 16;
  }
  qf->insertElement(qfv((starting_bucket + 3) % 16, remainders[3]));
  b[3] = (starting_bucket + 3) % 16;
  b[4] = (starting_bucket + 4) % 16;

  qf->deleteElement(qfv(starting_bucket, remainders[1]));
  qf->deleteElement(qfv(starting_bucket, remainders[2]));
  // Rebuild expected after 6 operations

  int expected_buckets[] = {b[0], b[1], b[3], b[4]};
  assert_empty_buckets(qf, 4, expected_buckets);
  Mdt slot_tests[] = {{b[0], true, false, false, false, (uint64_t)remainders[0]},
                       {b[1], false, true, true, true, psv(b[0], b[0])},
                       {b[3], true, false, false, false, (uint64_t)remainders[3]},
                       {b[4], false, true, true, true, psv(b[3], b[3])}};
  check_slots(qf, 4, slot_tests);
  EXPECT_EQ(qf->size, 2);
}

// Checks that single tombstones between runs are unaffected
TEST_F(RedistributionTestBetweenRunsInsert, RunGap) {
  int starting_bucket = 6;
  int remainders[] = {111, 222, 333, 444, 555};
  int b[6];

  for (int i = 0; i < 3; i ++) {
    qf->insertElement(qfv(starting_bucket, remainders[i]));
    b[i] = (starting_bucket + i) % 16;
  }
  for (int i = 3; i < 5; i ++) {
    qf->insertElement(qfv((starting_bucket + 1) % 16, remainders[i]));
    b[i] = (starting_bucket + i) % 16;
  }
  b[5] = (starting_bucket + 5) % 16;
  qf->deleteElement(qfv(starting_bucket, remainders[2]));
  // Rebuild expected after 6 operations

  Mdt slot_tests[] = {{b[0], true, false, false, false, (uint64_t)remainders[0]},
                       {b[1], true, true, true, false, (uint64_t)remainders[1]},
                       {b[2], false, true, true, true, psv(b[0], b[1])},
                       {b[3], false, true, false, false, (uint64_t)remainders[3]},
                       {b[4], false, true, true, false, (uint64_t)remainders[4]},
                       {b[5], false, true, true, true, psv(b[1], b[1])}};
  check_slots(qf, 6, slot_tests);
  EXPECT_EQ(qf->size, 4);
}

// Checks that multiple tombstones between runs are reduced to single tombstones
TEST_F(RedistributionTestBetweenRunsInsert, RunCleaned) {
  int starting_bucket = 12;
  int remainders[] = {111, 222, 333, 444};
  int b[4];

  for (int i = 0; i < 3; i ++) {
    qf->insertElement(qfv(starting_bucket, remainders[i]));
    b[i] = (starting_bucket + i) % 16;
  }
  b[3] = (starting_bucket + 3) % 16;
  qf->insertElement(qfv((starting_bucket + 1) % 16, remainders[3]));

  qf->deleteElement(qfv(starting_bucket, remainders[1]));
  qf->deleteElement(qfv(starting_bucket, remainders[2]));
  // Rebuild expected after 6 operations

  assert_empty_buckets(qf, 4, b);
  Mdt slot_tests[] = {{b[0], true, false, false, false, (uint64_t)remainders[0]},
                       {b[1], true, true, true, true, psv(b[0], b[1])},
                       {b[2], false, true, false, false, (uint64_t)remainders[3]},
                       {b[3], false, true, true, true, psv(b[1], b[1])}};
  check_slots(qf, 4, slot_tests);
  EXPECT_EQ(qf->size, 2);
}

// Checks that multiple tombstones between runs can be passed to or inserted in later runs
TEST_F(RedistributionTestBetweenRunsInsert, PassTombstones) {
  int starting_bucket = 4;
  int remainders[] = {111, 222, 333, 444, 555, 666, 777};
  int b[9];

  for (int i = 0; i < 4; i ++) {
    qf->insertElement(qfv(starting_bucket, remainders[i]));
    b[i] = (starting_bucket + i) % 16;
  }
  for (int i = 4; i < 6; i ++) {
    qf->insertElement(qfv((starting_bucket + 1) % 16, remainders[i]));
    b[i] = (starting_bucket + i) % 16;
  }
  // First rebuild expected after 6 operations; 2 tombstones inserted
  // load factor = 1/2 -> x = 1/(1/2) = 2 -> window size = 16/(4 * 2) = 2
  // -> rebuild interval 2

  for (int i = 6; i < 9; i ++) {
    b[i] = (starting_bucket + i) % 16;
  }
  qf->insertElement(qfv((starting_bucket + 2) % 16, remainders[6]));
  qf->deleteElement(qfv(starting_bucket, remainders[1]));

  assert_empty_buckets(qf, 9, b);
  Mdt slot_tests[] = {{b[0], true, false, false, false, (uint64_t)remainders[0]},
                      {b[1], true, true, true, false, (uint64_t)remainders[2]},
                      {b[2], true, true, true, false, (uint64_t)remainders[3]},
                      {b[3], false, true, true, true, psv(b[0], b[1])},
                      {b[4], false, true, false, false, (uint64_t)remainders[4]},
                      {b[5], false, true, true, false, (uint64_t)remainders[5]},
                      {b[6], false, true, true, true, psv(b[1], b[2])},
                      {b[7], false, true, false, false, (uint64_t)remainders[6]},
                      {b[8], false, true, true, true, psv(b[2], b[2])}};
  check_slots(qf, 9, slot_tests);
  EXPECT_EQ(qf->size, 6);
}


///////// Evenly Distribute Policy

// Checks that a rebuild evenly distributes tombstones between clusters
TEST_F(RedistributionTestEvenlyDistribute, InsertOnlySeparate) {
  int starting_bucket = 13;
  int remainders[] = {12, 323, 5942, 102, 3, 6};

  for (int i = 0; i < 6; i ++) {
    qf->insertElement(qfv((starting_bucket + i) % 16, remainders[i]));
  }
  // Rebuild expected after 6 operations
  // load factor = 3/8 -> x = 1/(5/8) = 8/5 -> #tombstones = 16/(2x) = 8 * 5/8 = 5; interval size = 3.2
  // insert at 0, 3(.2), 6(.4), 9(.6), 12(.8)

  int expected_buckets[] = {0, 1, 2, 3, 4, 6, 9, 12, 13, 14, 15};
  assert_empty_buckets(qf, 11, expected_buckets);
  Mdt2 slot_tests[] = {{0, true, false, false, false, false, (uint64_t)remainders[3]},
                      {1, true, true, true, true, false, psv(0, 1)},
                      {2, true, true, false, false, false, (uint64_t)remainders[4]},
                      {3, false, true, false, false, false, (uint64_t)remainders[5]},
                      {4, false, true, true, true, false, psv(3, 3)},
                      {6, false, false, false, true, true, psv(6, 6)},
                      {9, false, false, false, true, true, psv(9, 9)},
                      {12, false, false, false, true, true, psv(12, 12)},
                      {13, true, false, false, false, false, (uint64_t)remainders[0]},
                      {14, true, false, false, false, false, (uint64_t)remainders[1]},
                      {15, true, false, false, false, false, (uint64_t)remainders[2]}};
  check_slots(qf, 11, slot_tests);
  EXPECT_EQ(qf->size, 6);
}

// Checks that a rebuild that distributes tombstones into a single run (and clears tombstones between rebuilds)
TEST_F(RedistributionTestEvenlyDistribute, InsertOnlySingle) {
  int starting_bucket = 1;
  int remainders[] = {987, 65, 4, 321, 9, 8, 765, 43};

  for (int i = 0; i < 6; i ++) {
    qf->insertElement(qfv(starting_bucket, remainders[i]));
  }
  // Rebuild expected after 6 operations
  // load factor = 3/8 -> x = 1/(5/8) = 8/5 -> #tombstones = 16/(2x) = 8 * 5/8 = 5; interval size = 3.2
  // insert at 0, 3(.2), 6(.4), 9(.6), 12(.8)

  int expected_buckets[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 12};
  assert_empty_buckets(qf, 11, expected_buckets);
  Mdt2 slot_tests1[] = {{0, false, false, false, true, true, psv(0, 0)},
                        {1, true, false, false, false, false, (uint64_t)remainders[0]},
                        {2, false, true, true, false, false, (uint64_t)remainders[1]},
                        {3, false, true, true, false, false, (uint64_t)remainders[2]},
                        {4, false, true, true, false, false, (uint64_t)remainders[3]},
                        {5, false, true, true, false, false, (uint64_t)remainders[4]},
                        {6, false, true, true, false, false, (uint64_t)remainders[5]},
                        {7, false, true, true, true, false, psv(1, 1)},
                        {8, false, true, true, true, false, psv(1, 1)},
                        {9, false, false, false, true, true, psv(9, 9)},
                        {12, false, false, false, true, true, psv(12, 12)}};
  check_slots(qf, 11, slot_tests1);
  EXPECT_EQ(qf->size, 6);

  // x = 8/5 -> window size = 16/(4 * x) = 16/4 * 10/16 = 10/4 = 2.5
  // -> rebuild interval 2
  for (int i = 6; i < 8; i ++) {
    qf->insertElement(qfv(starting_bucket, remainders[i]));
  }
  // load factor = 1/2 -> x = 1/(1/2) = 2 -> #tombstones = 16/(2x) = 4; interval size = 4
  // insert at 0, 4, 8, 12

  int expected_buckets2[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12};
  assert_empty_buckets(qf, 12, expected_buckets2);
  Mdt2 slot_tests2[] = {{0, false, false, false, true, true, psv(0, 0)},
                        {1, true, false, false, false, false, (uint64_t)remainders[0]},
                        {2, false, true, true, false, false, (uint64_t)remainders[1]},
                        {3, false, true, true, false, false, (uint64_t)remainders[2]},
                        {4, false, true, true, false, false, (uint64_t)remainders[3]},
                        {5, false, true, true, false, false, (uint64_t)remainders[4]},
                        {6, false, true, true, false, false, (uint64_t)remainders[5]},
                        {7, false, true, true, false, false, (uint64_t)remainders[6]},
                        {8, false, true, true, false, false, (uint64_t)remainders[7]},
                        {9, false, true, true, true, false, psv(1, 1)},
                        {10, false, true, true, true, false, psv(1, 1)},
                        {12, false, false, false, true, true, psv(12, 12)}};
  check_slots(qf, 12, slot_tests2);
  EXPECT_EQ(qf->size, 8);
}

// Checks that a rebuild clears a table containing only tombstones before distributing
TEST_F(RedistributionTestEvenlyDistribute, TombstonesOnly) {
  int starting_bucket = 6;
  int remainders[] = {111, 222, 333};

  for (int i = 0; i < 3; i ++) {
    qf->insertElement(qfv((starting_bucket + i) % 16, remainders[i]));
  }
  for (int i = 0; i < 3; i ++) {
    qf->deleteElement(qfv((starting_bucket + i) % 16, remainders[i]));
  }
  // Rebuild expected after 6 operations
  // load factor = 0 -> x = 1 -> #tombstones = 16/(2x) = 8; interval size = 2
  // insert at 0, 2, 4, 6, 8, 10, 12, 14

  int b[8];
  Mdt2 slot_tests[8];
  for (int i = 0; i < 8; i ++) {
    b[i] = 2 * i;
    slot_tests[i] = {2 * i, false, false, false, true, true, psv(2*i, 2*i)};
  }
  assert_empty_buckets(qf, 8, b);
  check_slots(qf, 8, slot_tests);
  EXPECT_EQ(qf->size, 0);
}

// Checks that single tombstones between clusters are cleaned up
TEST_F(RedistributionTestEvenlyDistribute, ClusterGap) {
  int starting_bucket = 13;
  int remainders[] = {111, 222, 333, 444, 555};

  for (int i = 0; i < 3; i ++) {
    qf->insertElement(qfv(starting_bucket, remainders[i]));
  }
  qf->deleteElement(qfv(starting_bucket, remainders[2]));
  for (int i = 3; i < 5; i ++) {
    qf->insertElement(qfv((starting_bucket + 3) % 16, remainders[i]));
  }
  // Rebuild expected after 6 operations
  // load factor = 1/4 -> x = 1/(3/4) = 4/3 -> #tombstones = 16/(2x) = 16/2 * 12/16 = 6; interval size = 8/3
  // insert at 0, 2(.6), 5(.3), 8, 10(.6), 13(.3)

  Mdt2 slot_tests[] = {{13, true, false, false, false, false, (uint64_t)remainders[0]},
                       {14, false, true, true, false, false, (uint64_t)remainders[1]},
                       {15, false, true, true, true, false, psv(13, 13)},
                       {0, true, false, false, false, false, (uint64_t)remainders[3]},
                       {1, false, true, true, false, false, (uint64_t)remainders[4]},
                       {2, false, true, true, true, false, psv(0, 0)},
                       {3, false, true, true, true, false, psv(0, 0)},
                       {5, false, false, false, true, true, psv(5, 5)},
                       {8, false, false, false, true, true, psv(8, 8)},
                       {10, false, false, false, true, true, psv(10, 10)}};
  check_slots(qf, 10, slot_tests);
  EXPECT_EQ(qf->size, 4);
}

// Checks that multiple tombstones between clusters are cleaned up
TEST_F(RedistributionTestEvenlyDistribute, ClusterCleaned) {
  int starting_bucket = 2;
  int remainders[] = {111, 222, 333, 444};

  for (int i = 0; i < 3; i ++) {
    qf->insertElement(qfv(starting_bucket, remainders[i]));
  }
  qf->insertElement(qfv((starting_bucket + 3) % 16, remainders[3]));

  qf->deleteElement(qfv(starting_bucket, remainders[1]));
  qf->deleteElement(qfv(starting_bucket, remainders[2]));
  // Rebuild expected after 6 operations
  // load factor = 1/8 -> x = 1/(7/8) = 8/7 -> #tombstones = 16/(2x) = 8 * 7/8 = 7; interval size = 16/7
  // insert at 0, 2(.2), 4(.5), 6(.8), 9(.1), 11(.4), 13(.7)

  int expected_buckets[] = {0, 2, 3, 4, 5, 6, 9, 11, 13};
  assert_empty_buckets(qf, 9, expected_buckets);
  Mdt2 slot_tests[] = {{0, false, false, false, true, true, psv(0, 0)},
                       {2, true, false, false, false, false, (uint64_t)remainders[0]},
                       {3, false, true, true, true, false, psv(2, 2)},
                       {4, false, false, false, true, true, psv(4, 4)},
                       {5, true, false, false, false, false, (uint64_t)remainders[3]},
                       {6, false, false, false, true, true, psv(6, 6)},
                       {9, false, false, false, true, true, psv(9, 9)},
                       {11, false, false, false, true, true, psv(11, 11)},
                       {13, false, false, false, true, true, psv(13, 13)}};
  check_slots(qf, 9, slot_tests);
  EXPECT_EQ(qf->size, 2);
}

// Checks that single tombstones between runs are cleaned up
TEST_F(RedistributionTestEvenlyDistribute, RunGap) {
  int starting_bucket = 6;
  int remainders[] = {111, 222, 333, 444, 555};

  for (int i = 0; i < 3; i ++) {
    qf->insertElement(qfv(starting_bucket, remainders[i]));
  }
  for (int i = 3; i < 5; i ++) {
    qf->insertElement(qfv((starting_bucket + 1) % 16, remainders[i]));
  }
  qf->deleteElement(qfv(starting_bucket, remainders[2]));
  // Rebuild expected after 6 operations
  // load factor = 1/4 -> x = 1/(3/4) = 4/3 -> #tombstones = 16/(2x) = 16/2 * 12/16 = 6; interval size = 8/3
  // insert at 0, 2(.6), 5(.3), 8, 10(.6), 13(.3)

  Mdt2 slot_tests[] = {{0, false, false, false, true, true, psv(0, 0)},
                       {2, false, false, false, true, true, psv(2, 2)},
                       {5, false, false, false, true, true, psv(5, 5)},
                       {6, true, false, false, false, false, (uint64_t)remainders[0]},
                       {7, true, true, true, false, false, (uint64_t)remainders[1]},
                       {8, false, true, false, false, false, (uint64_t)remainders[3]},
                       {9, false, true, true, false, false, (uint64_t)remainders[4]},
                       {10, false, true, true, true, false, psv(7, 7)},
                       {11, false, true, true, true, false, psv(7, 7)},
                       {13, false, false, false, true, true, psv(13, 13)}};
  check_slots(qf, 10, slot_tests);
  EXPECT_EQ(qf->size, 4);
}

// Checks that multiple tombstones between runs are cleaned up
TEST_F(RedistributionTestEvenlyDistribute, RunCleaned) {
  int starting_bucket = 12;
  int remainders[] = {111, 222, 333, 444};

  for (int i = 0; i < 3; i ++) {
    qf->insertElement(qfv(starting_bucket, remainders[i]));
  }
  qf->insertElement(qfv((starting_bucket + 1) % 16, remainders[3]));

  qf->deleteElement(qfv(starting_bucket, remainders[1]));
  qf->deleteElement(qfv(starting_bucket, remainders[2]));
  // Rebuild expected after 6 operations
  // load factor = 1/8 -> x = 1/(7/8) = 8/7 -> #tombstones = 16/(2x) = 8 * 7/8 = 7; interval size = 16/7
  // insert at 0, 2(.2), 4(.5), 6(.8), 9(.1), 11(.4), 13(.7)

  int expected_buckets[] = {0, 2, 4, 6, 9, 11, 12, 13, 14};
  assert_empty_buckets(qf, 9, expected_buckets);
  Mdt2 slot_tests[] = {{0, false, false, false, true, true, psv(0, 0)},
                       {2, false, false, false, true, true, psv(2, 2)},
                       {4, false, false, false, true, true, psv(4, 4)},
                       {6, false, false, false, true, true, psv(6, 6)},
                       {9, false, false, false, true, true, psv(9, 9)},
                       {11, false, false, false, true, true, psv(11, 11)},
                       {12, true, false, false, false, false, (uint64_t)remainders[0]},
                       {13, true, false, false, false, false, (uint64_t)remainders[3]},
                       {14, false, true, true, true, true, psv(13, 13)}};
  check_slots(qf, 9, slot_tests);
  EXPECT_EQ(qf->size, 2);
}

// Checks that multiple tombstones between runs are cleaned up
TEST_F(RedistributionTestEvenlyDistribute, PassTombstonesKindOf) {
  int starting_bucket = 4;
  int remainders[] = {111, 222, 333, 444, 555, 666, 777};

  for (int i = 0; i < 4; i ++) {
    qf->insertElement(qfv(starting_bucket, remainders[i]));
  }
  for (int i = 4; i < 6; i ++) {
    qf->insertElement(qfv((starting_bucket + 1) % 16, remainders[i]));
  }
  // First rebuild expected after 6 operations; 2 tombstones inserted
  // load factor = 3/8 -> x = 1/(5/8) = 8/5 -> #tombstones = 16/(2x) = 8 * 5/8 = 5; interval size = 3.2
  // insert at 0, 3(.2), 6(.4), 9(.6), 12(.8)
  // x = 8/5 -> window size = 16/(4 * x) = 8/2 * 5/8 = 2.5
  // -> rebuild interval 2

  qf->insertElement(qfv((starting_bucket + 2) % 16, remainders[6]));
  qf->deleteElement(qfv(starting_bucket, remainders[1]));
  // load factor = 3/8 -> x = 1/(5/8) = 8/5 -> #tombstones = 16/(2x) = 8 * 5/8 = 5; interval size = 3.2
  // insert at 0, 3(.2), 6(.4), 9(.6), 12(.8)

  int expected_buckets[] = {0, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
  assert_empty_buckets(qf, 11, expected_buckets);
  Mdt2 slot_tests[] = {{0, false, false, false, true, true, psv(0, 0)},
                      {3, false, false, false, true, true, psv(0, 0)},
                      {4, true, false, false, false, false, (uint64_t)remainders[0]},
                      {5, true, true, true, false, false, (uint64_t)remainders[2]},
                      {6, true, true, true, false, false, (uint64_t)remainders[3]},
                      {7, false, true, true, true, false, psv(4, 5)},
                      {8, false, true, false, false, false, (uint64_t)remainders[4]},
                      {9, false, true, true, false, false, (uint64_t)remainders[5]},
                      {10, false, true, true, true, false, psv(5, 6)},
                      {11, false, true, false, false, false, (uint64_t)remainders[6]},
                      {12, false, true, true, true, false, psv(6, 7)}};
  check_slots(qf, 11, slot_tests);
  EXPECT_EQ(qf->size, 6);
}

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