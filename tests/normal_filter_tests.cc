#include <gtest/gtest.h>
#include "../quotient_filter/quotient_filter.h"

int identity(int x) {
    return x;
}

// Proof of concept test
TEST(HelloTest, InsertAndQuery) {
  QuotientFilter* qf = new QuotientFilter(4, &identity);

  qf->insertElement(2);
  EXPECT_TRUE(qf->query(2));

  free(qf);
}
