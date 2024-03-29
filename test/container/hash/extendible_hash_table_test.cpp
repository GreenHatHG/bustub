/**
 * extendible_hash_test.cpp
 */

#include <thread>  // NOLINT

#include "container/hash/extendible_hash_table.h"
#include "gtest/gtest.h"

namespace bustub {

TEST(ExtendibleHashTableTest, SampleTest) {
  auto table = std::make_unique<ExtendibleHashTable<int, std::string>>(2);

  table->Insert(1, "a");
  table->Insert(2, "b");
  table->Insert(3, "c");
  table->Insert(4, "d");
  table->Insert(5, "e");
  table->Insert(6, "f");
  table->Insert(7, "g");
  table->Insert(8, "h");
  table->Insert(9, "i");
  EXPECT_EQ(2, table->GetLocalDepth(0));
  EXPECT_EQ(3, table->GetLocalDepth(1));
  EXPECT_EQ(2, table->GetLocalDepth(2));
  EXPECT_EQ(2, table->GetLocalDepth(3));

  std::string result;
  table->Find(9, result);
  EXPECT_EQ("i", result);
  table->Find(8, result);
  EXPECT_EQ("h", result);
  table->Find(2, result);
  EXPECT_EQ("b", result);
  EXPECT_FALSE(table->Find(10, result));

  EXPECT_TRUE(table->Remove(8));
  EXPECT_TRUE(table->Remove(4));
  EXPECT_TRUE(table->Remove(1));
  EXPECT_FALSE(table->Remove(20));
}

TEST(ExtendibleHashTableTest, ConcurrentInsertTest) {
  const int num_runs = 50;
  const int num_threads = 3;

  // Run concurrent test multiple times to guarantee correctness.
  for (int run = 0; run < num_runs; run++) {
    auto table = std::make_unique<ExtendibleHashTable<int, int>>(2);
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (int tid = 0; tid < num_threads; tid++) {
      threads.emplace_back([tid, &table]() { table->Insert(tid, tid); });
    }
    for (int i = 0; i < num_threads; i++) {
      threads[i].join();
    }

    EXPECT_EQ(table->GetGlobalDepth(), 1);
    for (int i = 0; i < num_threads; i++) {
      int val;
      EXPECT_TRUE(table->Find(i, val));
      EXPECT_EQ(i, val);
    }
  }
}

TEST(ExtendibleHashTableTest, SampleTest2) {
  auto table = std::make_unique<ExtendibleHashTable<int, std::string>>(2);
  // => global_depth_ = 0, bucket_size_ = 2, num_buckets_ = 1
  std::string result;

  table->Insert(1, "a");
  EXPECT_TRUE(table->Find(1, result));
  table->Insert(2, "b");
  EXPECT_TRUE(table->Find(2, result));
  table->Insert(3, "c");
  EXPECT_TRUE(table->Find(3, result));
  table->Insert(4, "d");
  EXPECT_TRUE(table->Find(4, result));
  table->Insert(5, "e");
  EXPECT_TRUE(table->Find(5, result));
  table->Insert(6, "f");
  EXPECT_TRUE(table->Find(6, result));
  table->Insert(7, "g");
  EXPECT_TRUE(table->Find(7, result));
  table->Insert(8, "h");
  EXPECT_TRUE(table->Find(8, result));
  table->Insert(9, "i");
  EXPECT_TRUE(table->Find(9, result));

  EXPECT_EQ(2, table->GetLocalDepth(0));
  EXPECT_EQ(3, table->GetLocalDepth(1));
  EXPECT_EQ(2, table->GetLocalDepth(2));
  EXPECT_EQ(2, table->GetLocalDepth(3));

  table->Find(9, result);
  EXPECT_EQ("i", result);
  table->Find(8, result);
  EXPECT_EQ("h", result);
  table->Find(2, result);
  EXPECT_EQ("b", result);
  EXPECT_FALSE(table->Find(10, result));

  EXPECT_TRUE(table->Remove(8));
  EXPECT_TRUE(table->Remove(4));
  EXPECT_TRUE(table->Remove(1));
  EXPECT_FALSE(table->Remove(20));
}

TEST(ExtendibleHashTableTest, InsertSplit) {
  auto table = std::make_unique<ExtendibleHashTable<int, std::string>>(2);
  EXPECT_EQ(0, table->GetGlobalDepth());
  EXPECT_EQ(1, table->GetNumBuckets());
  table->Insert(1, "a");
  table->Insert(2, "b");
  table->Insert(3, "c");
  table->Insert(4, "d");
  table->Insert(5, "e");
  table->Insert(6, "f");
  EXPECT_EQ(2, table->GetGlobalDepth());
}

TEST(ExtendibleHashTableTest, InsertMultipleSplit) {
  auto table = std::make_unique<ExtendibleHashTable<int, int>>(2);
  table->Insert(0, 0);
  table->Insert(1024, 1024);
  table->Insert(4, 4);
  EXPECT_EQ(4, table->GetNumBuckets());
}

TEST(ExtendibleHashTableTest, ConcurrentInsertTest2) {
  const int num_runs = 50;
  const int num_threads = 3;

  // Run concurrent test multiple times to guarantee correctness.
  for (int run = 0; run < num_runs; run++) {
    auto table = std::make_unique<ExtendibleHashTable<int, int>>(2);
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (int tid = 0; tid < num_threads; tid++) {
      threads.emplace_back([tid, &table]() { table->Insert(tid, tid); });
      // table->Insert(tid, tid);
    }
    for (int i = 0; i < num_threads; i++) {
      threads[i].join();
    }

    EXPECT_EQ(table->GetGlobalDepth(), 1);
    for (int i = 0; i < num_threads; i++) {
      int val;
      EXPECT_TRUE(table->Find(i, val));
      EXPECT_EQ(i, val);
    }
  }
}

TEST(ExtendibleHashTableTest, GetNumBuckets) {
  auto table = std::make_unique<ExtendibleHashTable<int, int>>(2);
  table->Insert(4, 0);
  table->Insert(12, 0);
  table->Insert(16, 0);
  table->Insert(64, 0);
  table->Insert(31, 0);
  table->Insert(10, 0);

  table->Insert(51, 0);
  table->Insert(15, 0);
  table->Insert(18, 0);
  table->Insert(20, 0);

  table->Insert(7, 0);
  table->Insert(23, 0);
  table->Insert(11, 0);
  table->Insert(19, 0);
}

TEST(ExtendibleHashTableTest, GetNumBuckets2) {
  auto table = std::make_unique<ExtendibleHashTable<int, std::string>>(4);

  table->Insert(4, "a");
  table->Insert(12, "a");
  table->Insert(16, "a");
  table->Insert(64, "a");
  table->Insert(31, "a");
  table->Insert(10, "a");
  table->Insert(51, "a");
  table->Insert(15, "a");
  table->Insert(18, "a");
  table->Insert(20, "a");
  table->Insert(7, "a");
  table->Insert(23, "a");

  EXPECT_EQ(6, table->GetNumBuckets());
}

TEST(ExtendibleHashTableTest, GRADER_LocalDepth) {
  auto table = std::make_unique<ExtendibleHashTable<int, int>>(4);
  int result;

  table->Insert(4, 4);
  table->Insert(12, 12);
  table->Insert(16, 16);
  table->Insert(64, 64);
  table->Insert(5, 5);
  table->Insert(10, 10);
  table->Insert(51, 51);
  table->Insert(15, 15);
  table->Insert(18, 18);
  table->Insert(20, 20);
  table->Insert(7, 7);
  table->Insert(21, 21);

  EXPECT_EQ(2, table->GetLocalDepth(5));

  table->Insert(11, 11);
  table->Insert(19, 19);
  EXPECT_TRUE(table->Find(15, result));
}

TEST(ExtendibleHashTableTest, LocalDepth) {
  auto table = std::make_unique<ExtendibleHashTable<int, int>>(4);

  table->Insert(4, 4);
  table->Insert(12, 12);
  table->Insert(16, 16);
  table->Insert(64, 64);
  table->Insert(5, 5);
  table->Insert(10, 10);
  table->Insert(51, 51);
  table->Insert(15, 15);
  table->Insert(18, 18);
  table->Insert(20, 20);
  table->Insert(7, 7);
  table->Insert(21, 21);
  table->Insert(11, 11);
  table->Insert(19, 19);

  EXPECT_EQ(3, table->GetLocalDepth(3));
}

TEST(ExtendibleHashTableTest, InsertFind) {
  auto table = std::make_unique<ExtendibleHashTable<int, int>>(2);

  table->Insert(1, 1);
  table->Insert(2, 2);
  table->Insert(3, 3);
  table->Insert(4, 4);
  table->Insert(5, 5);
  table->Insert(6, 6);
  table->Insert(7, 7);
  table->Insert(8, 8);
  table->Insert(9, 9);
  table->Insert(10, 10);
  table->Insert(20, 20);
  table->Insert(11, 11);
  table->Insert(12, 12);
  table->Insert(13, 13);
  table->Insert(14, 14);
  table->Insert(15, 15);
  table->Insert(16, 16);
  table->Insert(17, 17);
  table->Insert(18, 18);
  table->Insert(19, 19);
  table->Insert(30, 30);
  table->Insert(21, 21);
  table->Insert(22, 22);
  table->Insert(23, 23);
  table->Insert(24, 24);
  table->Insert(25, 25);
  table->Insert(26, 26);
  table->Insert(27, 27);
  table->Insert(28, 28);
  table->Insert(31, 31);
  table->Insert(32, 32);
  table->Insert(33, 33);
  table->Insert(34, 34);
  table->Insert(35, 35);
  table->Insert(36, 36);
  table->Insert(37, 37);
}

}  // namespace bustub
