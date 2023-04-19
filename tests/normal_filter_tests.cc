#include <gtest/gtest.h>
#include "../quotient_filter/quotient_filter.h"

int identity(int x) {
    return x;
}

class QuotientFilterTest : public ::testing::Test {
  protected:
    QuotientFilter* qf;

    void SetUp() override {
      qf = new QuotientFilter(4, &identity);
    }

    // void TearDown() override {}
};

// Proof of concept test
TEST_F(QuotientFilterTest, InsertAndQuery) {
  qf->insertElement(2);
  EXPECT_TRUE(qf->query(2));
}
