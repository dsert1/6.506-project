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
class RedistributionTest : public ::testing::Test {
  protected:
    QuotientFilterGraveyard* qf;

    void SetUp() override {
      qf = new QuotientFilterGraveyard(4, &identity, no_redistribution);
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



///////// Empty Filter Tests

// Checks that the quotient filter is initalized with expected values
TEST_F(RedistributionTest, FilterConstruction) {
  EXPECT_EQ(qf->size, 0);
  EXPECT_EQ(qf->q, 4);
  EXPECT_EQ(qf->r, 28);
  EXPECT_EQ(qf->table_size, 16);
}

// Testing contents of a filter with no elements inserted
TEST_F(RedistributionTest, EmptyFilter) {
  assert_empty_buckets(qf, 0, NULL);
}



///////// Single Element Tests

// Testing contents of a filter with a single element inserted
TEST_F(RedistributionTest, SingleElementInsert) {
  int test_bucket = 9;
  int test_remainder = 2;

  qf->insertElement(qfv(test_bucket, test_remainder));
  int expected_bucket[] = {test_bucket};
  assert_empty_buckets(qf, 1, expected_bucket);

  EXPECT_TRUE(qf->query(qfv(test_bucket, test_remainder)));
  EXPECT_FALSE(qf->query(qfv(test_bucket, test_remainder + 1)));
  Mdt slot_tests[] = {{test_bucket, true, false, false, false, (uint64_t)test_remainder}};
  check_slots(qf, 1, slot_tests);
  EXPECT_EQ(qf->size, 1);
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