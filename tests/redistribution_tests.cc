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

// Fixture class
class RedistributionTestBetweenRuns : public ::testing::Test {
  protected:
    QuotientFilterGraveyard* qf;

    void SetUp() override {
      qf = new QuotientFilterGraveyard(4, &identity, between_runs);
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

// Checks that multiple tombstones between runs can be passed to later tombstones
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