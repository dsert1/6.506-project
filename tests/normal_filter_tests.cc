#include <gtest/gtest.h>
#include "../quotient_filter/quotient_filter.h"
#include <bitset>
// disables a warning for converting ints to uint64_t
#pragma warning( disable: 4838 )

// To be used as the hash function for testing
int identity(int x) {
    return x;
}

bool validateTable;

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
    if (validateTable) {
      QuotientFilterElement elt = qf->table[bucket];
      EXPECT_EQ(elt.is_occupied, 0);
      EXPECT_EQ(elt.is_shifted, 0);
      EXPECT_EQ(elt.is_continuation, 0);
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
  uint64_t value;
} Mdt;

void check_slots(QuotientFilter* qf, int test_count, Mdt metadata_tests[]) {
  if (!validateTable) {
    return;
  }

  for (int i = 0; i < test_count; i++) {
    Mdt test = metadata_tests[i];
    QuotientFilterElement elt = qf->table[test.slot];

    EXPECT_EQ(elt.is_occupied, test.is_occupied);
    EXPECT_EQ(elt.is_shifted, test.is_shifted);
    EXPECT_EQ(elt.is_continuation, test.is_continuation);
    EXPECT_EQ(elt.value, test.value);
  }
}

// debug printing
void scan_table(QuotientFilter* qf) {
  for (int b = 0; b < 16; b++) {
    QuotientFilterElement elt = qf->table[b];
    std::cerr << "[table debug]" << b << ": " <<
      elt.is_occupied << elt.is_shifted << elt.is_continuation << ", " << elt.value << "\n";
  }
}



///////// Empty Filter Tests

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



///////// Single Element Tests

// Testing contents of a filter with a single element inserted
TEST_F(QuotientFilterTest, SingleElementInsert) {
  int test_bucket = 9;
  int test_remainder = 2;

  qf->insertElement(qfv(test_bucket, test_remainder));
  int expected_bucket[] = {test_bucket};
  assert_empty_buckets(qf, 1, expected_bucket);

  EXPECT_TRUE(qf->query(qfv(test_bucket, test_remainder)));
  EXPECT_FALSE(qf->query(qfv(test_bucket, test_remainder + 1)));
  Mdt slot_tests[] = {{test_bucket, true, false, false, (uint64_t)test_remainder}};
  check_slots(qf, 1, slot_tests);
  EXPECT_EQ(qf->size, 1);
}

// Testing deleting elements in a filter with a single element
TEST_F(QuotientFilterTest, SingleElementDelete) {
  int test_bucket = 2;
  int test_remainder = 193;

  qf->insertElement(qfv(test_bucket, test_remainder));
  qf->deleteElement(qfv(test_bucket, test_remainder));

  EXPECT_FALSE(qf->query(qfv(test_bucket, test_remainder)));
  assert_empty_buckets(qf, 0, NULL);
  EXPECT_EQ(qf->size, 0);
}



///////// Two Element Tests

// Testing inserting two separated elements
TEST_F(QuotientFilterTest, TwoSeparateElementsInsert) {
  int first_bucket = 0;
  int first_remainder = 92;
  int second_bucket = 11;
  int second_remainder = 0;

  qf->insertElement(qfv(second_bucket, second_remainder));
  qf->insertElement(qfv(first_bucket, first_remainder));
  int expected_buckets[] = {first_bucket, second_bucket};
  assert_empty_buckets(qf, 2, expected_buckets);

  EXPECT_TRUE(qf->query(qfv(first_bucket, first_remainder)));
  EXPECT_TRUE(qf->query(qfv(second_bucket, second_remainder)));
  Mdt slot_tests[] = {{first_bucket, true, false, false, (uint64_t)first_remainder},
                      {second_bucket, true, false, false, (uint64_t)second_remainder}};
  check_slots(qf, 2, slot_tests);
  EXPECT_EQ(qf->size, 2);
}

// Testing deleting two separated elements
TEST_F(QuotientFilterTest, TwoSeparateElementsDelete) {
  int first_bucket = 3;
  int first_remainder = 5;
  int second_bucket = 5;
  int second_remainder = 3;

  qf->insertElement(qfv(first_bucket, first_remainder));
  qf->insertElement(qfv(second_bucket, second_remainder));

  qf->deleteElement(qfv(second_bucket, second_remainder));
  EXPECT_TRUE(qf->query(qfv(first_bucket, first_remainder)));
  EXPECT_FALSE(qf->query(qfv(second_bucket, second_remainder)));
  EXPECT_EQ(qf->size, 1);

  qf->deleteElement(qfv(first_bucket, first_remainder));
  EXPECT_FALSE(qf->query(qfv(first_bucket, first_remainder)));
  EXPECT_FALSE(qf->query(qfv(second_bucket, second_remainder)));
  assert_empty_buckets(qf, 0, NULL);
  EXPECT_EQ(qf->size, 0);
}

// Testing inserting two adjacent elements
TEST_F(QuotientFilterTest, TwoAdjacentElementsInsert) {
  int first_bucket = 8;
  int first_remainder = 3;
  int second_bucket = 9;
  int second_remainder = 3;

  qf->insertElement(qfv(second_bucket, second_remainder));
  EXPECT_FALSE(qf->query(qfv(first_bucket, first_remainder)));
  qf->insertElement(qfv(first_bucket, first_remainder));
  int expected_buckets[] = {first_bucket, second_bucket};
  assert_empty_buckets(qf, 2, expected_buckets);

  EXPECT_TRUE(qf->query(qfv(first_bucket, first_remainder)));
  EXPECT_TRUE(qf->query(qfv(second_bucket, second_remainder)));
  Mdt slot_tests[] = {{first_bucket, true, false, false, (uint64_t)first_remainder},
                      {second_bucket, true, false, false, (uint64_t)second_remainder}};
  check_slots(qf, 2, slot_tests);
  EXPECT_EQ(qf->size, 2);
}

// Testing deleting two adjacent elements
TEST_F(QuotientFilterTest, TwoAdjacentElementsDelete) {
  int first_bucket = 12;
  int first_remainder = 12;
  int second_bucket = 13;
  int second_remainder = 21;

  qf->insertElement(qfv(first_bucket, first_remainder));
  qf->insertElement(qfv(second_bucket, second_remainder));

  qf->deleteElement(qfv(first_bucket, first_remainder));
  EXPECT_FALSE(qf->query(qfv(first_bucket, first_remainder)));
  EXPECT_TRUE(qf->query(qfv(second_bucket, second_remainder)));
  EXPECT_EQ(qf->size, 1);

  qf->deleteElement(qfv(second_bucket, second_remainder));
  EXPECT_FALSE(qf->query(qfv(first_bucket, first_remainder)));
  EXPECT_FALSE(qf->query(qfv(second_bucket, second_remainder)));
  assert_empty_buckets(qf, 0, NULL);
  EXPECT_EQ(qf->size, 0);
}



///////// Run Tests

// Inserting elements to create one run
TEST_F(QuotientFilterTest, SingleRunInsert) {
  int bucket = 7;
  int remainders[] = {102, 39, 5920};
  int not_remainder = 555;

  int rb2 = (bucket + 1) % 16;
  int rb3 = (bucket + 2) % 16;

  for (int i = 0; i < 3; i++) {
    qf->insertElement(qfv(bucket, remainders[i]));
  }
  int expected_buckets[] = {bucket, rb2, rb3};
  assert_empty_buckets(qf, 3, expected_buckets);

  EXPECT_FALSE(qf->query(qfv(bucket, not_remainder)));
  for (int i = 0; i < 3; i ++) {
    EXPECT_TRUE(qf->query(qfv(bucket, remainders[i])));
  }
  Mdt slot_tests[] = {{bucket, true, false, false, (uint64_t)remainders[0]},
                      {rb2, false, true, true, (uint64_t)remainders[1]},
                      {rb3, false, true, true, (uint64_t)remainders[2]}};
  check_slots(qf, 3, slot_tests);
  EXPECT_EQ(qf->size, 3);
}

// Deleting elements from a run in the order they were inserted
TEST_F(QuotientFilterTest, SingleRunDelete) {
  int bucket = 10;
  int remainders[] = {0, 15, 6};

  int rb2 = (bucket + 1) % 16;

  for (int i = 0; i < 3; i++) {
    qf->insertElement(qfv(bucket, remainders[i]));
    EXPECT_TRUE(qf->query(qfv(bucket, remainders[i])));
  }

  qf->deleteElement(qfv(bucket, remainders[0]));
  EXPECT_FALSE(qf->query(qfv(bucket, remainders[0])));
  EXPECT_TRUE(qf->query(qfv(bucket, remainders[1])));
  EXPECT_TRUE(qf->query(qfv(bucket, remainders[2])));
  int expected_buckets[] = {bucket, rb2};
  assert_empty_buckets(qf, 2, expected_buckets);
  Mdt slot_tests[] = {{bucket, true, false, false, (uint64_t)remainders[1]},
                      {rb2, false, true, true, (uint64_t)remainders[2]}};
  check_slots(qf, 2, slot_tests);
  EXPECT_EQ(qf->size, 2);

  qf->deleteElement(qfv(bucket, remainders[1]));
  qf->deleteElement(qfv(bucket, remainders[2]));
  EXPECT_FALSE(qf->query(qfv(bucket, remainders[1])));
  EXPECT_FALSE(qf->query(qfv(bucket, remainders[2])));
  assert_empty_buckets(qf, 0, NULL);
  EXPECT_EQ(qf->size, 0);
}

// Deleting elements from a run opposite the order they were inserted
TEST_F(QuotientFilterTest, SingleRunDeleteReverse) {
  int bucket = 2;
  int remainders[] = {998, 2, 534};

  int rb2 = (bucket + 1) % 16;

  for (int i = 0; i < 3; i++) {
    qf->insertElement(qfv(bucket, remainders[i]));
    EXPECT_TRUE(qf->query(qfv(bucket, remainders[i])));
  }

  qf->deleteElement(qfv(bucket, remainders[2]));
  EXPECT_TRUE(qf->query(qfv(bucket, remainders[0])));
  EXPECT_TRUE(qf->query(qfv(bucket, remainders[1])));
  EXPECT_FALSE(qf->query(qfv(bucket, remainders[2])));
  int expected_buckets[] = {bucket, rb2};
  assert_empty_buckets(qf, 2, expected_buckets);
  Mdt slot_tests[] = {{bucket, true, false, false, (uint64_t)remainders[0]},
                      {rb2, false, true, true, (uint64_t)remainders[1]}};
  check_slots(qf, 2, slot_tests);
  EXPECT_EQ(qf->size, 2);

  qf->deleteElement(qfv(bucket, remainders[1]));
  qf->deleteElement(qfv(bucket, remainders[0]));
  EXPECT_FALSE(qf->query(qfv(bucket, remainders[0])));
  EXPECT_FALSE(qf->query(qfv(bucket, remainders[1])));
  assert_empty_buckets(qf, 0, NULL);
  EXPECT_EQ(qf->size, 0);
}

// Testing a run that wraps around the array
TEST_F(QuotientFilterTest, SingleRunWrapAround) {
  int remainders[] = {360, 720, 1080, 1440};

  for (int i = 0; i < 4; i++) {
    qf->insertElement(qfv(14, remainders[i]));
    EXPECT_TRUE(qf->query(qfv(14, remainders[i])));
  }

  int expected_buckets[] = {0, 1, 14, 15};
  assert_empty_buckets(qf, 4, expected_buckets);
  Mdt slot_tests[] = {{14, true, false, false, (uint64_t)remainders[0]},
                      {15, false, true, true, (uint64_t)remainders[1]},
                      {0, false, true, true, (uint64_t)remainders[2]},
                      {1, false, true, true, (uint64_t)remainders[3]}};
  check_slots(qf, 4, slot_tests);
  EXPECT_EQ(qf->size, 4);

  qf->deleteElement(qfv(14, remainders[1]));
  qf->deleteElement(qfv(14, remainders[3]));
  EXPECT_TRUE(qf->query(qfv(14, remainders[0])));
  EXPECT_FALSE(qf->query(qfv(14, remainders[1])));
  EXPECT_TRUE(qf->query(qfv(14, remainders[2])));
  EXPECT_FALSE(qf->query(qfv(14, remainders[3])));
  EXPECT_EQ(qf->size, 2);

  qf->deleteElement(qfv(14, remainders[0]));
  qf->deleteElement(qfv(14, remainders[2]));
  EXPECT_FALSE(qf->query(qfv(14, remainders[0])));
  EXPECT_FALSE(qf->query(qfv(14, remainders[2])));
  assert_empty_buckets(qf, 0, NULL);
  EXPECT_EQ(qf->size, 0);
}



///////// Cluster Tests

// Inserting elements in order to create cluster of two runs
TEST_F(QuotientFilterTest, DoubleRunInsert) {
  int first_bucket = 4;
  int remainders[] = {1010, 202, 333, 4040};

  int second_bucket = (first_bucket + 1) % 16;
  int buckets[] = {first_bucket, first_bucket, second_bucket, second_bucket};
  int rb2 = (first_bucket + 1) % 16;
  int rb3 = (first_bucket + 2) % 16;
  int rb4 = (first_bucket + 3) % 16;

  for (int i = 0; i < 4; i++) {
    qf->insertElement(qfv(buckets[i], remainders[i]));
  }
  int expected_buckets[] = {first_bucket, rb2, rb3, rb4};
  assert_empty_buckets(qf, 4, expected_buckets);

  for (int i = 0; i < 4; i ++) {
    EXPECT_TRUE(qf->query(qfv(buckets[i], remainders[i])));
  }
  EXPECT_FALSE(qf->query(qfv(first_bucket, remainders[2])));
  Mdt slot_tests[] = {{first_bucket, true, false, false, (uint64_t)remainders[0]},
                      {rb2, true, true, true, (uint64_t)remainders[1]},
                      {rb3, false, true, false, (uint64_t)remainders[2]},
                      {rb4, false, true, true, (uint64_t)remainders[3]}};
  check_slots(qf, 4, slot_tests);
  EXPECT_EQ(qf->size, 4);
}

// Inserting elements to create cluster of two runs in reverse order
TEST_F(QuotientFilterTest, DoubleRunInsertReverse) {
  int first_bucket = 9;
  int remainders[] = {9303, 1233, 7452, 2345};

  int second_bucket = (first_bucket + 1) % 16;
  int buckets[] = {second_bucket, second_bucket, first_bucket, first_bucket};
  int rb2 = (first_bucket + 1) % 16;
  int rb3 = (first_bucket + 2) % 16;
  int rb4 = (first_bucket + 3) % 16;

  for (int i = 0; i < 4; i++) {
    qf->insertElement(qfv(buckets[i], remainders[i]));
  }
  int expected_buckets[] = {first_bucket, rb2, rb3, rb4};
  assert_empty_buckets(qf, 4, expected_buckets);

  for (int i = 0; i < 4; i ++) {
    EXPECT_TRUE(qf->query(qfv(buckets[i], remainders[i])));
  }
  Mdt slot_tests[] = {{first_bucket, true, false, false, (uint64_t)remainders[2]},
                      {rb2, true, true, true, (uint64_t)remainders[3]},
                      {rb3, false, true, false, (uint64_t)remainders[0]},
                      {rb4, false, true, true, (uint64_t)remainders[1]}};
  check_slots(qf, 4, slot_tests);
  EXPECT_EQ(qf->size, 4);
}

// Inserting elements to create cluster of two runs in an interleaved order
TEST_F(QuotientFilterTest, DoubleRunInsertInterleaved) {
  int buckets[] = {15, 14, 14, 15};
  int remainders[] = {1111, 22, 333, 4};

  for (int i = 0; i < 4; i++) {
    qf->insertElement(qfv(buckets[i], remainders[i]));
    std::cerr << '\n';
  }
  int expected_buckets[] = {0, 1, 14, 15};
  assert_empty_buckets(qf, 4, expected_buckets);

  for (int i = 0; i < 4; i ++) {
    EXPECT_TRUE(qf->query(qfv(buckets[i], remainders[i])));
  }
  Mdt slot_tests[] = {{14, true, false, false, (uint64_t)remainders[1]},
                      {15, true, true, true, (uint64_t)remainders[2]},
                      {0, false, true, false, (uint64_t)remainders[0]},
                      {1, false, true, true, (uint64_t)remainders[3]}};
  check_slots(qf, 4, slot_tests);
  EXPECT_EQ(qf->size, 4);
}

// Deleting cluster of two runs, starting with the first run
TEST_F(QuotientFilterTest, DoubleRunDelete) {
  int first_bucket = 0;
  int remainders[] = {314, 159, 265, 358};

  int second_bucket = (first_bucket + 1) % 16;
  int buckets[] = {first_bucket, first_bucket, second_bucket, second_bucket};
  int rb2 = (first_bucket + 1) % 16;
  int rb3 = (first_bucket + 2) % 16;

  for (int i = 0; i < 4; i++) {
    qf->insertElement(qfv(buckets[i], remainders[i]));
    EXPECT_TRUE(qf->query(qfv(buckets[i], remainders[i])));
  }

  qf->deleteElement(qfv(buckets[0], remainders[0]));
  qf->deleteElement(qfv(buckets[1], remainders[1]));
  EXPECT_FALSE(qf->query(qfv(buckets[0], remainders[0])));
  EXPECT_FALSE(qf->query(qfv(buckets[1], remainders[1])));
  EXPECT_TRUE(qf->query(qfv(buckets[2], remainders[2])));
  EXPECT_TRUE(qf->query(qfv(buckets[3], remainders[3])));

  int expected_buckets[] = {rb2, rb3};
  assert_empty_buckets(qf, 2, expected_buckets);
  Mdt slot_tests[] = {{rb2, true, false, false, (uint64_t)remainders[2]},
                      {rb3, false, true, true, (uint64_t)remainders[3]}};
  check_slots(qf, 2, slot_tests);
  EXPECT_EQ(qf->size, 2);
}

// Deleting cluster of two runs, starting with the second run
TEST_F(QuotientFilterTest, DoubleRunDeleteReverse) {
  int first_bucket = 8;
  int remainders[] = {5245, 0, 1123, 56};

  int second_bucket = (first_bucket + 1) % 16;
  int buckets[] = {first_bucket, first_bucket, second_bucket, second_bucket};
  int rb2 = (first_bucket + 1) % 16;

  for (int i = 0; i < 4; i++) {
    qf->insertElement(qfv(buckets[i], remainders[i]));
    EXPECT_TRUE(qf->query(qfv(buckets[i], remainders[i])));
  }

  qf->deleteElement(qfv(buckets[2], remainders[2]));
  qf->deleteElement(qfv(buckets[3], remainders[3]));
  EXPECT_TRUE(qf->query(qfv(buckets[0], remainders[0])));
  EXPECT_TRUE(qf->query(qfv(buckets[1], remainders[1])));
  EXPECT_FALSE(qf->query(qfv(buckets[2], remainders[2])));
  EXPECT_FALSE(qf->query(qfv(buckets[3], remainders[3])));

  int expected_buckets[] = {first_bucket, rb2};
  assert_empty_buckets(qf, 2, expected_buckets);
  Mdt slot_tests[] = {{first_bucket, true, false, false, (uint64_t)remainders[0]},
                      {rb2, false, true, true, (uint64_t)remainders[1]}};
  check_slots(qf, 2, slot_tests);
  EXPECT_EQ(qf->size, 2);
}

// General test with longer runs
TEST_F(QuotientFilterTest, DoubleRunLonger) {
  int first_bucket = 13;
  int remainders[] = {123, 456, 789, 12, 345, 678, 901, 234, 567, 890};

  int second_bucket = (first_bucket + 3) % 16;
  int buckets[] = {first_bucket, first_bucket, second_bucket, second_bucket,
                   first_bucket, first_bucket, second_bucket, second_bucket,
                   second_bucket, first_bucket};

  int b[10];
  for (int i = 0; i < 10; i++) {
    qf->insertElement(qfv(buckets[i], remainders[i]));
    b[i] = (first_bucket + i) % 16;
  }
  EXPECT_EQ(qf->size, 10);

  for (int i = 0; i < 10; i ++) {
    EXPECT_TRUE(qf->query(qfv(buckets[i], remainders[i])));
  }
  Mdt slot_tests1[] = {{b[0], true, false, false, (uint64_t)remainders[0]},
                      {b[1], false, true, true, (uint64_t)remainders[1]},
                      {b[2], false, true, true, (uint64_t)remainders[4]},
                      {b[3], true, true, true, (uint64_t)remainders[5]},
                      {b[4], false, true, true, (uint64_t)remainders[9]},
                      {b[5], false, true, false, (uint64_t)remainders[2]},
                      {b[6], false, true, true, (uint64_t)remainders[3]},
                      {b[7], false, true, true, (uint64_t)remainders[6]},
                      {b[8], false, true, true, (uint64_t)remainders[7]},
                      {b[9], false, true, true, (uint64_t)remainders[8]}};
  check_slots(qf, 10, slot_tests1);

  qf->deleteElement(qfv(buckets[1], remainders[1]));
  qf->deleteElement(qfv(buckets[7], remainders[7]));
  qf->deleteElement(qfv(buckets[4], remainders[4]));
  for (int i = 0; i < 10; i ++) {
    if (i % 3 == 1) {
      EXPECT_FALSE(qf->query(qfv(buckets[i], remainders[i])));
    } else {
      EXPECT_TRUE(qf->query(qfv(buckets[i], remainders[i])));
    }
  }
  Mdt slot_tests2[] = {{b[0], true, false, false, (uint64_t)remainders[0]},
                      {b[1], false, true, true, (uint64_t)remainders[5]},
                      {b[2], false, true, true, (uint64_t)remainders[9]},
                      {b[3], true, false, false, (uint64_t)remainders[2]},
                      {b[4], false, true, true, (uint64_t)remainders[3]},
                      {b[5], false, true, true, (uint64_t)remainders[6]},
                      {b[6], false, true, true, (uint64_t)remainders[8]}};
  check_slots(qf, 7, slot_tests2);
  EXPECT_EQ(qf->size, 7);
}

// Testing with runs overlapping a cluster
TEST_F(QuotientFilterTest, TestInterruptionSimple) {
  int first_bucket = 3;
  int remainders[] = {103, 194, 128, 349, 301, 392};
  int b_remainder = 222;
  int d_remainder = 444;

  int ba = first_bucket;
  int bb = (first_bucket + 1) % 16;
  int bc = (first_bucket + 2) % 16;
  int bd = (first_bucket + 3) % 16;
  int buckets[] = {ba, ba, ba, bc, bc, bc};

  int b[8];
  for (int i = 0; i < 8; i ++) {
    if (i < 6) {
      qf->insertElement(qfv(buckets[i], remainders[i]));
    }
    b[i] = (first_bucket + i) % 16;
  }
  EXPECT_EQ(qf->size, 6);

  qf->insertElement(qfv(bb, b_remainder));
  EXPECT_TRUE(qf->query(qfv(bb, b_remainder)));
  qf->insertElement(qfv(bd, d_remainder));
  EXPECT_TRUE(qf->query(qfv(bd, d_remainder)));

  Mdt slot_tests1[] = {{b[0], true, false, false, (uint64_t)remainders[0]},
                       {b[1], true, true, true, (uint64_t)remainders[1]},
                       {b[2], true, true, true, (uint64_t)remainders[2]},
                       {b[3], true, true, false, (uint64_t)b_remainder},
                       {b[4], false, true, false, (uint64_t)remainders[3]},
                       {b[5], false, true, true, (uint64_t)remainders[4]},
                       {b[6], false, true, true, (uint64_t)remainders[5]},
                       {b[7], false, true, false, (uint64_t)d_remainder}};
  check_slots(qf, 8, slot_tests1);
  EXPECT_EQ(qf->size, 8);

  for (int i = 0; i < 6; i ++) {
    qf->deleteElement(qfv(buckets[i], remainders[i]));
  }
  EXPECT_TRUE(qf->query(qfv(bb, b_remainder)));
  EXPECT_TRUE(qf->query(qfv(bd, d_remainder)));
  Mdt slot_tests2[] = {{bb, true, false, false, (uint64_t)b_remainder},
                       {bd, true, false, false, (uint64_t)d_remainder}};
  check_slots(qf, 2, slot_tests2);
  EXPECT_EQ(qf->size, 2);
}

// Testing the invariant-threatening example
TEST_F(QuotientFilterTest, TestInterruptionTricky) {
  int first_bucket = 12;
  int remainders[] = {103, 194, 128, 192, 349, 301, 392};
  int b_remainder = 222;
  int d_remainder = 444;

  int ba = first_bucket;
  int bb = (first_bucket + 1) % 16;
  int bc = (first_bucket + 2) % 16;
  int bd = (first_bucket + 3) % 16;
  int buckets[] = {ba, ba, ba, ba, bc, bc, bc};

  int b[9];
  for (int i = 0; i < 9; i ++) {
    if (i < 7) {
      qf->insertElement(qfv(buckets[i], remainders[i]));
    }
    b[i] = (first_bucket + i) % 16;
  }
  EXPECT_EQ(qf->size, 7);

  qf->insertElement(qfv(bd, d_remainder));
  EXPECT_TRUE(qf->query(qfv(bd, d_remainder)));
  qf->insertElement(qfv(bb, b_remainder));
  EXPECT_TRUE(qf->query(qfv(bb, b_remainder)));

  Mdt slot_tests1[] = {{b[0], true, false, false, (uint64_t)remainders[0]},
                       {b[1], true, true, true, (uint64_t)remainders[1]},
                       {b[2], true, true, true, (uint64_t)remainders[2]},
                       {b[3], true, true, true, (uint64_t)remainders[3]},
                       {b[4], false, true, false, (uint64_t)b_remainder},
                       {b[5], false, true, false, (uint64_t)remainders[4]},
                       {b[6], false, true, true, (uint64_t)remainders[5]},
                       {b[7], false, true, true, (uint64_t)remainders[6]},
                       {b[8], false, true, false, (uint64_t)d_remainder}};
  check_slots(qf, 9, slot_tests1);
  EXPECT_EQ(qf->size, 9);


  for (int i = 0; i < 7; i ++) {
    qf->deleteElement(qfv(buckets[i], remainders[i]));
  }
  EXPECT_TRUE(qf->query(qfv(bb, b_remainder)));
  EXPECT_TRUE(qf->query(qfv(bd, d_remainder)));
  Mdt slot_tests2[] = {{bb, true, false, false, (uint64_t)b_remainder},
                       {bd, true, false, false, (uint64_t)d_remainder}};
  check_slots(qf, 2, slot_tests2);
  EXPECT_EQ(qf->size, 2);
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