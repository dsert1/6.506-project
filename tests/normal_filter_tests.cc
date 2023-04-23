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

// Proof of concept test
TEST_F(QuotientFilterTest, InsertAndQuery) {
  qf->insertElement(qfv(9, 2));
  std::cerr << "testing; " << sizeof(int);
  EXPECT_TRUE(qf->query(qfv(9, 2)));
}
