#include <gtest/gtest.h>
#include "../quotient_filter/quotient_filter.h"

// To be used as the hash function for testing
int identity(int x) {
    return x;
}

// Fixture class
class QuotientFilterTest : public ::testing::Test {
  protected:
    QuotientFilter* qf;

    void SetUp() override {
      qf = new QuotientFilter(4, &identity);
    }

    // void TearDown() override {}
};

// Builds a value to insert into the filter, based on q and r
// q should be less than 16
int qfv(int q, int r) {
  return ((q & 15) << 28) + r;
}

// Checks to see that all buckets are empty, except for the given exceptions
// exceptions should be sorted
void assert_empty_buckets(QuotientFilter* qf, int exceptionCount, int exceptions[]) {
  int ce = 0;
  for (int bucket = 0; bucket < 16; bucket++) {
    if (ce < exceptionCount && exceptions[ce] == bucket) {
      ce++;
      continue;
    }

    EXPECT_FALSE(qf->query(qfv(bucket, 0)));
    QuotientFilterElement elt = qf->table[bucket];
    EXPECT_EQ(elt.is_occupied, 0);
    EXPECT_EQ(elt.is_shifted, 0);
    EXPECT_EQ(elt.is_continuation, 0);
    EXPECT_EQ(elt.value, 0);
  }
}

// Proof of concept test (likely to be deleted later)
TEST_F(QuotientFilterTest, InsertAndQuery) {
  qf->insertElement(qfv(9, 2));
  std::cerr << "testing; " << sizeof(int) << "\n";
  EXPECT_TRUE(qf->query(qfv(9, 2)));
}




// Checks that the quotient filter is initalized with expected values
TEST_F(QuotientFilterTest, FilterConstruction) {
  EXPECT_EQ(qf->size, 0);
  EXPECT_EQ(qf->q, 4);
  EXPECT_EQ(qf->r, 28);
  EXPECT_EQ(qf->table_size, 16);
}

// Testing contents of a filter with no elements inserted
TEST_F(QuotientFilterTest, EmptyFilter) {
  assert_empty_buckets(qf, 0, NULL);
}

// Testing contents of a filter with a single element inserted
TEST_F(QuotientFilterTest, SingleElementInsert) {
  int test_bucket = 9;
  int test_remainder = 2;

  qf->insertElement(qfv(test_bucket, test_remainder));
  int expected_bucket[] = {test_bucket};
  assert_empty_buckets(qf, 1, expected_bucket);

  EXPECT_TRUE(qf->query(qfv(test_bucket, test_remainder)));
  QuotientFilterElement elt = qf->table[test_bucket];
  EXPECT_EQ(elt.is_occupied, 1);
  EXPECT_EQ(elt.is_shifted, 0);
  EXPECT_EQ(elt.is_continuation, 0);
  EXPECT_EQ(elt.value, test_remainder);
}

// Testing deleting elements in a filter with a single element
TEST_F(QuotientFilterTest, SingleElementDelete) {
  int test_bucket = 9;
  int test_remainder = 2;

  qf->insertElement(qfv(test_bucket, test_remainder));
  qf->deleteElement(qfv(test_bucket, test_remainder));

  EXPECT_FALSE(qf->query(qfv(test_bucket, test_remainder)));
  assert_empty_buckets(qf, 0, NULL);
}
