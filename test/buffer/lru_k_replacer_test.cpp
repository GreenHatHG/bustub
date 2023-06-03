/**
 * lru_k_replacer_test.cpp
 */

#include "buffer/lru_k_replacer.h"

#include <algorithm>
#include <cstdio>
#include <memory>
#include <random>
#include <set>
#include <thread>  // NOLINT
#include <vector>

#include "gtest/gtest.h"

namespace bustub {

TEST(LRUKReplacerTest, SampleTest) {
  LRUKReplacer lru_replacer(7, 2);

  // Scenario: add six elements to the replacer. We have [1,2,3,4,5]. Frame 6 is non-evictable.
  lru_replacer.RecordAccess(1);
  lru_replacer.RecordAccess(2);
  lru_replacer.RecordAccess(3);
  lru_replacer.RecordAccess(4);
  lru_replacer.RecordAccess(5);
  lru_replacer.RecordAccess(6);
  lru_replacer.SetEvictable(1, true);
  lru_replacer.SetEvictable(2, true);
  lru_replacer.SetEvictable(3, true);
  lru_replacer.SetEvictable(4, true);
  lru_replacer.SetEvictable(5, true);
  lru_replacer.SetEvictable(6, false);
  ASSERT_EQ(5, lru_replacer.Size());

  // Scenario: Insert access history for frame 1. Now frame 1 has two access histories.
  // All other frames have max backward k-dist. The order of eviction is [2,3,4,5,1].
  lru_replacer.RecordAccess(1);

  // Scenario: Evict three pages from the replacer. Elements with max k-distance should be popped
  // first based on LRU.
  int value;
  lru_replacer.Evict(&value);
  ASSERT_EQ(2, value);
  lru_replacer.Evict(&value);
  ASSERT_EQ(3, value);
  lru_replacer.Evict(&value);
  ASSERT_EQ(4, value);
  ASSERT_EQ(2, lru_replacer.Size());

  // Scenario: Now replacer has frames [5,1].
  // Insert new frames 3, 4, and update access history for 5. We should end with [3,1,5,4]
  lru_replacer.RecordAccess(3);
  lru_replacer.RecordAccess(4);
  lru_replacer.RecordAccess(5);
  lru_replacer.RecordAccess(4);
  lru_replacer.SetEvictable(3, true);
  lru_replacer.SetEvictable(4, true);
  ASSERT_EQ(4, lru_replacer.Size());

  // Scenario: continue looking for victims. We expect 3 to be evicted next.
  lru_replacer.Evict(&value);
  ASSERT_EQ(3, value);
  ASSERT_EQ(3, lru_replacer.Size());

  // Set 6 to be evictable. 6 Should be evicted next since it has max backward k-dist.
  lru_replacer.SetEvictable(6, true);
  ASSERT_EQ(4, lru_replacer.Size());
  lru_replacer.Evict(&value);
  ASSERT_EQ(6, value);
  ASSERT_EQ(3, lru_replacer.Size());

  // Now we have [1,5,4]. Continue looking for victims.
  lru_replacer.SetEvictable(1, false);
  ASSERT_EQ(2, lru_replacer.Size());
  ASSERT_EQ(true, lru_replacer.Evict(&value));
  ASSERT_EQ(5, value);
  ASSERT_EQ(1, lru_replacer.Size());

  // Update access history for 1. Now we have [4,1]. Next victim is 4.
  lru_replacer.RecordAccess(1);
  lru_replacer.RecordAccess(1);
  lru_replacer.SetEvictable(1, true);
  ASSERT_EQ(2, lru_replacer.Size());
  ASSERT_EQ(true, lru_replacer.Evict(&value));
  ASSERT_EQ(value, 4);

  ASSERT_EQ(1, lru_replacer.Size());
  lru_replacer.Evict(&value);
  ASSERT_EQ(value, 1);
  ASSERT_EQ(0, lru_replacer.Size());

  // These operations should not modify size
  ASSERT_EQ(false, lru_replacer.Evict(&value));
  ASSERT_EQ(0, lru_replacer.Size());
  lru_replacer.Remove(1);
  ASSERT_EQ(0, lru_replacer.Size());
}

TEST(LRUKReplacerTest, SampleTest2) {
  LRUKReplacer lru_replacer(7, 2);

  // 添加五个元素到replacer中
  lru_replacer.RecordAccess(1);
  lru_replacer.RecordAccess(2);
  lru_replacer.RecordAccess(3);
  lru_replacer.RecordAccess(4);
  lru_replacer.RecordAccess(5);
  lru_replacer.SetEvictable(1, true);
  lru_replacer.SetEvictable(2, true);
  lru_replacer.SetEvictable(3, true);
  lru_replacer.SetEvictable(4, true);
  lru_replacer.SetEvictable(5, true);
  ASSERT_EQ(5, lru_replacer.Size());
  lru_replacer.RecordAccess(4);
  lru_replacer.RecordAccess(1);

  int value;
  ASSERT_EQ(true, lru_replacer.Evict(&value));

  ASSERT_EQ(2, value);

  lru_replacer.RecordAccess(1);
  // 让1跑到k链表最前面

  ASSERT_EQ(4, lru_replacer.Size());

  lru_replacer.SetEvictable(3, false);
  lru_replacer.SetEvictable(5, false);

  ASSERT_EQ(2, lru_replacer.Size());

  ASSERT_EQ(true, lru_replacer.Evict(&value));

  ASSERT_EQ(4, value);

  ASSERT_EQ(1, lru_replacer.Size());

  lru_replacer.RecordAccess(1);

  lru_replacer.RecordAccess(2);

  ASSERT_EQ(1, lru_replacer.Size());

  ASSERT_EQ(true, lru_replacer.Evict(&value));

  ASSERT_EQ(1, value);
  ASSERT_EQ(0, lru_replacer.Size());

  lru_replacer.RecordAccess(3);

  ASSERT_EQ(false, lru_replacer.Evict(&value));

  lru_replacer.SetEvictable(3, true);
  lru_replacer.SetEvictable(5, true);

  ASSERT_EQ(true, lru_replacer.Evict(&value));

  ASSERT_EQ(5, value);

  lru_replacer.RecordAccess(2);

  ASSERT_EQ(1, lru_replacer.Size());

  lru_replacer.RecordAccess(3);
  lru_replacer.RecordAccess(3);

  ASSERT_EQ(true, lru_replacer.Evict(&value));

  ASSERT_EQ(3, value);

  ASSERT_EQ(0, lru_replacer.Size());

  ASSERT_EQ(false, lru_replacer.Evict(&value));

  lru_replacer.SetEvictable(2, true);
  ASSERT_EQ(1, lru_replacer.Size());
  ASSERT_EQ(true, lru_replacer.Evict(&value));

  ASSERT_EQ(2, value);

  ASSERT_EQ(0, lru_replacer.Size());
}

TEST(LRUKReplacerTest, SampleTest3) {
  LRUKReplacer lru_replacer(7, 1);

  // 添加五个元素到replacer中
  lru_replacer.RecordAccess(1);
  lru_replacer.RecordAccess(2);
  lru_replacer.RecordAccess(3);
  lru_replacer.RecordAccess(4);
  lru_replacer.RecordAccess(5);
  lru_replacer.SetEvictable(1, true);
  lru_replacer.SetEvictable(2, true);
  lru_replacer.SetEvictable(3, true);
  lru_replacer.SetEvictable(4, true);
  lru_replacer.SetEvictable(5, true);
  ASSERT_EQ(5, lru_replacer.Size());
  lru_replacer.RecordAccess(4);
  lru_replacer.RecordAccess(1);
  int value;
  ASSERT_EQ(true, lru_replacer.Evict(&value));
  ASSERT_EQ(2, value);

  ASSERT_EQ(true, lru_replacer.Evict(&value));
  ASSERT_EQ(3, value);

  ASSERT_EQ(true, lru_replacer.Evict(&value));
  ASSERT_EQ(5, value);

  lru_replacer.SetEvictable(4, false);

  ASSERT_EQ(true, lru_replacer.Evict(&value));
  ASSERT_EQ(1, value);

  ASSERT_EQ(0, lru_replacer.Size());

  lru_replacer.RecordAccess(5);
  ASSERT_EQ(0, lru_replacer.Size());
  lru_replacer.SetEvictable(5, true);
  ASSERT_EQ(1, lru_replacer.Size());

  ASSERT_EQ(true, lru_replacer.Evict(&value));
  ASSERT_EQ(5, value);

  ASSERT_EQ(false, lru_replacer.Evict(&value));

  lru_replacer.SetEvictable(4, true);
  ASSERT_EQ(true, lru_replacer.Evict(&value));
  ASSERT_EQ(4, value);
  ASSERT_EQ(0, lru_replacer.Size());
}

TEST(LRUKReplacerTest, SampleTest4) {
  LRUKReplacer lru_replacer(7, 2);

  // Scenario: add six elements to the replacer. We have [1,2,3,4,5]. Frame 6 is non-evictable.
  lru_replacer.RecordAccess(1);
  lru_replacer.RecordAccess(2);
  lru_replacer.RecordAccess(3);
  lru_replacer.RecordAccess(4);
  lru_replacer.RecordAccess(5);
  lru_replacer.RecordAccess(6);
  lru_replacer.SetEvictable(1, true);
  lru_replacer.SetEvictable(2, true);
  lru_replacer.SetEvictable(3, true);
  lru_replacer.SetEvictable(4, true);
  lru_replacer.SetEvictable(5, true);
  lru_replacer.SetEvictable(6, false);
  ASSERT_EQ(5, lru_replacer.Size());

  // Scenario: Insert access history for frame 1. Now frame 1 has two access histories.
  // All other frames have max backward k-dist. The order of eviction is [2,3,4,5,1].
  lru_replacer.RecordAccess(1);

  // Scenario: Evict three pages from the replacer. Elements with max k-distance should be popped
  // first based on LRU.
  int value;
  lru_replacer.Evict(&value);
  ASSERT_EQ(2, value);
  lru_replacer.Evict(&value);
  ASSERT_EQ(3, value);
  lru_replacer.Evict(&value);
  ASSERT_EQ(4, value);
  ASSERT_EQ(2, lru_replacer.Size());

  // Scenario: Now replacer has frames [5,1].
  // Insert new frames 3, 4, and update access history for 5. We should end with [3,1,5,4]
  lru_replacer.RecordAccess(3);
  lru_replacer.RecordAccess(4);
  lru_replacer.RecordAccess(5);
  lru_replacer.RecordAccess(4);
  lru_replacer.SetEvictable(3, true);
  lru_replacer.SetEvictable(4, true);
  ASSERT_EQ(4, lru_replacer.Size());

  // Scenario: continue looking for victims. We expect 3 to be evicted next.
  lru_replacer.Evict(&value);
  ASSERT_EQ(3, value);
  ASSERT_EQ(3, lru_replacer.Size());

  // Set 6 to be evictable. 6 Should be evicted next since it has max backward k-dist.
  lru_replacer.SetEvictable(6, true);
  ASSERT_EQ(4, lru_replacer.Size());
  lru_replacer.Evict(&value);
  ASSERT_EQ(6, value);
  ASSERT_EQ(3, lru_replacer.Size());

  // Now we have [1,5,4]. Continue looking for victims.
  lru_replacer.SetEvictable(1, false);
  ASSERT_EQ(2, lru_replacer.Size());
  ASSERT_EQ(true, lru_replacer.Evict(&value));
  ASSERT_EQ(5, value);
  ASSERT_EQ(1, lru_replacer.Size());

  // Update access history for 1. Now we have [4,1]. Next victim is 4.
  lru_replacer.RecordAccess(1);
  lru_replacer.RecordAccess(1);
  lru_replacer.SetEvictable(1, true);
  ASSERT_EQ(2, lru_replacer.Size());
  ASSERT_EQ(true, lru_replacer.Evict(&value));
  ASSERT_EQ(value, 4);

  ASSERT_EQ(1, lru_replacer.Size());
  lru_replacer.Evict(&value);
  ASSERT_EQ(value, 1);
  ASSERT_EQ(0, lru_replacer.Size());

  // These operations should not modify size
  ASSERT_EQ(false, lru_replacer.Evict(&value));
  ASSERT_EQ(0, lru_replacer.Size());
  lru_replacer.Remove(1);
  ASSERT_EQ(0, lru_replacer.Size());
}

TEST(LRUKReplacerTest, Evict1) {
  // Empty and try removing
  LRUKReplacer lru_replacer(10, 2);
  int result;
  auto success = lru_replacer.Evict(&result);
  ASSERT_EQ(success, false) << "Check your return value behavior for LRUKReplacer::Evict";
}

TEST(LRUKReplacerTest, Evict2) {
  // Can only evict element if evictable=true
  int result;
  LRUKReplacer lru_replacer(10, 2);
  lru_replacer.RecordAccess(2);
  lru_replacer.SetEvictable(2, false);
  ASSERT_EQ(false, lru_replacer.Evict(&result)) << "Check your return value behavior for LRUKReplacer::Evict";
  lru_replacer.SetEvictable(2, true);
  ASSERT_EQ(true, lru_replacer.Evict(&result)) << "Check your return value behavior for LRUKReplacer::Evict";
  ASSERT_EQ(2, result) << "Check your return value behavior for LRUKReplacer::Evict";
}

TEST(LRUKReplacerTest, Evict3) {
  // Elements with less than k history should have max backward k-dist and get evicted first based on LRU
  LRUKReplacer lru_replacer(10, 3);
  int result;
  // 1 has three access histories, where as 2 has two access histories
  lru_replacer.RecordAccess(1);
  lru_replacer.RecordAccess(1);
  lru_replacer.RecordAccess(2);
  lru_replacer.RecordAccess(1);
  lru_replacer.SetEvictable(2, true);
  lru_replacer.SetEvictable(1, true);

  ASSERT_EQ(true, lru_replacer.Evict(&result)) << "Check your return value behavior for LRUKReplacer::Evict";
  ASSERT_EQ(2, result) << "Check your return value behavior for LRUKReplacer::Evict";
  ASSERT_EQ(true, lru_replacer.Evict(&result)) << "Check your return value behavior for LRUKReplacer::Evict";
  ASSERT_EQ(1, result) << "Check your return value behavior for LRUKReplacer::Evict";
}

TEST(LRUKReplacerTest, Evict4) {
  // Select element with largest backward k-dist to evict
  // Evicted page should not maintain previous history
  LRUKReplacer lru_replacer(10, 3);
  int result;
  lru_replacer.RecordAccess(1);
  lru_replacer.RecordAccess(2);
  lru_replacer.RecordAccess(3);
  lru_replacer.RecordAccess(3);
  lru_replacer.RecordAccess(3);
  lru_replacer.RecordAccess(2);
  lru_replacer.RecordAccess(2);
  lru_replacer.RecordAccess(1);
  lru_replacer.RecordAccess(1);
  lru_replacer.RecordAccess(3);
  lru_replacer.RecordAccess(2);
  lru_replacer.RecordAccess(1);
  lru_replacer.SetEvictable(2, true);
  lru_replacer.SetEvictable(1, true);
  lru_replacer.SetEvictable(3, true);

  // Should evict in this order
  ASSERT_EQ(true, lru_replacer.Evict(&result)) << "Check your return value behavior for LRUKReplacer::Evict";
  ASSERT_EQ(3, result) << "Check your return value behavior for LRUKReplacer::Evict";
  ASSERT_EQ(true, lru_replacer.Evict(&result)) << "Check your return value behavior for LRUKReplacer::Evict";
  ASSERT_EQ(2, result) << "Check your return value behavior for LRUKReplacer::Evict";
  ASSERT_EQ(true, lru_replacer.Evict(&result)) << "Check your return value behavior for LRUKReplacer::Evict";
  ASSERT_EQ(1, result) << "Check your return value behavior for LRUKReplacer::Evict";
}

TEST(LRUKReplacerTest, Evict5) {
  // Evicted page should not maintain previous history
  LRUKReplacer lru_replacer(10, 3);
  int result;
  lru_replacer.RecordAccess(2);
  lru_replacer.RecordAccess(2);
  lru_replacer.RecordAccess(2);
  lru_replacer.RecordAccess(1);
  lru_replacer.RecordAccess(1);
  lru_replacer.SetEvictable(2, true);
  lru_replacer.SetEvictable(1, true);

  // At this point, page 1 should be evicted since it has higher backward k distance
  ASSERT_EQ(true, lru_replacer.Evict(&result)) << "Check your return value behavior for LRUKReplacer::Evict";
  ASSERT_EQ(1, result) << "Check your return value behavior for LRUKReplacer::Evict";

  lru_replacer.RecordAccess(1);
  lru_replacer.SetEvictable(1, true);

  // 1 should still be evicted since it has max backward k distance
  ASSERT_EQ(true, lru_replacer.Evict(&result)) << "Check your return value behavior for LRUKReplacer::Evict";
  ASSERT_EQ(1, result) << "Check your return value behavior for LRUKReplacer::Evict";
}

TEST(LRUKReplacerTest, Evict6) {
  LRUKReplacer lru_replacer(10, 3);
  int result;
  lru_replacer.RecordAccess(1);  // ts=0
  lru_replacer.RecordAccess(2);  // ts=1
  lru_replacer.RecordAccess(3);  // ts=2
  lru_replacer.RecordAccess(4);  // ts=3
  lru_replacer.RecordAccess(1);  // ts=4
  lru_replacer.RecordAccess(2);  // ts=5
  lru_replacer.RecordAccess(3);  // ts=6
  lru_replacer.RecordAccess(1);  // ts=7
  lru_replacer.RecordAccess(2);  // ts=8
  lru_replacer.SetEvictable(1, true);
  lru_replacer.SetEvictable(2, true);
  lru_replacer.SetEvictable(3, true);
  lru_replacer.SetEvictable(4, true);

  // Max backward k distance follow lru
  ASSERT_EQ(true, lru_replacer.Evict(&result)) << "Check your return value behavior for LRUKReplacer::Evict";
  ASSERT_EQ(3, result) << "Check your return value behavior for LRUKReplacer::Evict";
  lru_replacer.RecordAccess(4);  // ts=9
  lru_replacer.RecordAccess(4);  // ts=10

  // Now 1 has largest backward k distance, followed by 2 and 4
  ASSERT_EQ(true, lru_replacer.Evict(&result)) << "Check your return value behavior for LRUKReplacer::Evict";
  ASSERT_EQ(1, result) << "Check your return value behavior for LRUKReplacer::Evict";
  ASSERT_EQ(true, lru_replacer.Evict(&result)) << "Check your return value behavior for LRUKReplacer::Evict";
  ASSERT_EQ(2, result) << "Check your return value behavior for LRUKReplacer::Evict";
  ASSERT_EQ(true, lru_replacer.Evict(&result)) << "Check your return value behavior for LRUKReplacer::Evict";
  ASSERT_EQ(4, result) << "Check your return value behavior for LRUKReplacer::Evict";
}

TEST(LRUKReplacerTest, Evict7) {
  // New unused page with max backward k-dist should be evicted first
  LRUKReplacer lru_replacer(10, 2);
  int result;
  lru_replacer.RecordAccess(1);  // ts=0
  lru_replacer.RecordAccess(2);  // ts=1
  lru_replacer.RecordAccess(3);  // ts=2
  lru_replacer.RecordAccess(4);  // ts=3
  lru_replacer.RecordAccess(1);  // ts=4
  lru_replacer.RecordAccess(2);  // ts=5
  lru_replacer.RecordAccess(3);  // ts=6
  lru_replacer.RecordAccess(4);  // ts=7

  lru_replacer.SetEvictable(2, true);
  lru_replacer.SetEvictable(1, true);

  ASSERT_EQ(true, lru_replacer.Evict(&result)) << "Check your return value behavior for LRUKReplacer::Evict";
  ASSERT_EQ(1, result) << "Check your return value behavior for LRUKReplacer::Evict";

  lru_replacer.RecordAccess(5);  // ts=9
  lru_replacer.SetEvictable(5, true);
  ASSERT_EQ(true, lru_replacer.Evict(&result)) << "Check your return value behavior for LRUKReplacer::Evict";
  ASSERT_EQ(5, result) << "Check your return value behavior for LRUKReplacer::Evict";
}

TEST(LRUKReplacerTest, Evict8) {
  // 1/4 page has one access history, 1/4 has two accesses, 1/4 has three, and 1/4 has four
  LRUKReplacer lru_replacer(1000, 3);
  int result;
  for (int j = 0; j < 4; ++j) {
    for (int i = j * 250; i < 1000; ++i) {
      lru_replacer.RecordAccess(i);
      lru_replacer.SetEvictable(i, true);
    }
  }
  ASSERT_EQ(1000, lru_replacer.Size());

  // Set second 1/4 to be non-evictable
  for (int i = 250; i < 500; ++i) {
    lru_replacer.SetEvictable(i, false);
  }
  ASSERT_EQ(750, lru_replacer.Size());

  // Remove first 100 elements
  for (int i = 0; i < 100; ++i) {
    lru_replacer.Remove(i);
  }
  ASSERT_EQ(650, lru_replacer.Size());

  // Try to evict some elements
  for (int i = 100; i < 600; ++i) {
    if (i < 250 || i >= 500) {
      ASSERT_EQ(true, lru_replacer.Evict(&result)) << "Check your return value behavior for LRUKReplacer::Evict";
      ASSERT_EQ(i, result) << "Check your return value behavior for LRUKReplacer::Evict";
    }
  }
  ASSERT_EQ(400, lru_replacer.Size());

  // Add second 1/4 elements back and modify access history for the last 150 elements of third 1/4 elements.
  for (int i = 250; i < 500; ++i) {
    lru_replacer.SetEvictable(i, true);
  }
  ASSERT_EQ(650, lru_replacer.Size());
  for (int i = 600; i < 750; ++i) {
    lru_replacer.RecordAccess(i);
    lru_replacer.RecordAccess(i);
  }
  ASSERT_EQ(650, lru_replacer.Size());

  // We expect the following eviction pattern
  for (int i = 250; i < 500; ++i) {
    ASSERT_EQ(true, lru_replacer.Evict(&result)) << "Check your return value behavior for LRUKReplacer::Evict";
    ASSERT_EQ(i, result) << "Check your return value behavior for LRUKReplacer::Evict";
  }
  ASSERT_EQ(400, lru_replacer.Size());
  for (int i = 750; i < 1000; ++i) {
    ASSERT_EQ(true, lru_replacer.Evict(&result)) << "Check your return value behavior for LRUKReplacer::Evict";
    ASSERT_EQ(i, result) << "Check your return value behavior for LRUKReplacer::Evict";
  }
  ASSERT_EQ(150, lru_replacer.Size());
  for (int i = 600; i < 750; ++i) {
    ASSERT_EQ(true, lru_replacer.Evict(&result)) << "Check your return value behavior for LRUKReplacer::Evict";
    ASSERT_EQ(i, result) << "Check your return value behavior for LRUKReplacer::Evict";
  }
  ASSERT_EQ(0, lru_replacer.Size());
}

TEST(LRUKReplacerTest, Size1) {
  // Size is increased/decreased if SetEvictable's argument is different from node state
  LRUKReplacer lru_replacer(10, 2);
  lru_replacer.RecordAccess(1);
  lru_replacer.SetEvictable(1, true);
  ASSERT_EQ(1, lru_replacer.Size()) << "Check your return value for LRUKReplacer::Size";
  lru_replacer.SetEvictable(1, true);
  ASSERT_EQ(1, lru_replacer.Size()) << "Check your return value for LRUKReplacer::Size";
  lru_replacer.SetEvictable(1, false);
  ASSERT_EQ(0, lru_replacer.Size()) << "Check your return value for LRUKReplacer::Size";
  lru_replacer.SetEvictable(1, false);
  ASSERT_EQ(0, lru_replacer.Size()) << "Check your return value for LRUKReplacer::Size";
}

TEST(LRUKReplacerTest, Size2) {
  // Insert new history. Calling SetEvictable = false should not modify Size.
  // Calling SetEvictable = true should increase Size.
  // Size should only be called when SetEvictable is called for every inserted node.
  LRUKReplacer lru_replacer(10, 2);
  lru_replacer.RecordAccess(1);
  lru_replacer.RecordAccess(2);
  lru_replacer.RecordAccess(3);
  lru_replacer.SetEvictable(1, false);
  lru_replacer.SetEvictable(2, false);
  lru_replacer.SetEvictable(3, false);
  ASSERT_EQ(0, lru_replacer.Size()) << "Check your return value for LRUKReplacer::Size";

  LRUKReplacer lru_replacer2(10, 2);
  lru_replacer2.RecordAccess(1);
  lru_replacer2.RecordAccess(2);
  lru_replacer2.RecordAccess(3);
  lru_replacer2.SetEvictable(1, true);
  lru_replacer2.SetEvictable(2, true);
  lru_replacer2.SetEvictable(3, true);
  ASSERT_EQ(3, lru_replacer2.Size()) << "Check your return value for LRUKReplacer::Size";
}

// Size depends on how many nodes have evictable=true
TEST(LRUKReplacerTest, Size3) {
  LRUKReplacer lru_replacer(10, 2);
  lru_replacer.RecordAccess(1);
  lru_replacer.RecordAccess(2);
  lru_replacer.RecordAccess(3);
  lru_replacer.RecordAccess(4);
  lru_replacer.RecordAccess(1);
  lru_replacer.RecordAccess(2);
  lru_replacer.RecordAccess(3);
  lru_replacer.RecordAccess(4);
  lru_replacer.RecordAccess(1);
  lru_replacer.RecordAccess(2);
  lru_replacer.RecordAccess(3);
  lru_replacer.RecordAccess(4);
  lru_replacer.RecordAccess(1);
  lru_replacer.RecordAccess(2);
  lru_replacer.RecordAccess(3);
  lru_replacer.RecordAccess(4);
  lru_replacer.SetEvictable(1, false);
  lru_replacer.SetEvictable(2, false);
  lru_replacer.SetEvictable(3, false);
  lru_replacer.SetEvictable(4, false);
  ASSERT_EQ(0, lru_replacer.Size()) << "Check your return value for LRUKReplacer::Size";
  lru_replacer.RecordAccess(1);
  lru_replacer.RecordAccess(2);
  lru_replacer.RecordAccess(3);
  lru_replacer.RecordAccess(4);
  lru_replacer.RecordAccess(1);
  lru_replacer.RecordAccess(2);
  lru_replacer.RecordAccess(3);
  lru_replacer.RecordAccess(4);
  lru_replacer.SetEvictable(1, true);
  lru_replacer.SetEvictable(2, true);
  lru_replacer.SetEvictable(1, true);
  lru_replacer.SetEvictable(2, true);
  ASSERT_EQ(2, lru_replacer.Size()) << "Check your return value for LRUKReplacer::Size";
  // Evicting a page should decrement Size
  lru_replacer.RecordAccess(4);
}

TEST(LRUKReplacerTest, Size4) {
  // Remove a page to decrement its size
  LRUKReplacer lru_replacer(10, 2);
  lru_replacer.RecordAccess(1);
  lru_replacer.SetEvictable(1, true);
  lru_replacer.RecordAccess(2);
  lru_replacer.SetEvictable(2, true);
  lru_replacer.RecordAccess(3);
  lru_replacer.SetEvictable(3, true);
  ASSERT_EQ(3, lru_replacer.Size()) << "Check your return value for LRUKReplacer::Size";
  lru_replacer.Remove(1);
  ASSERT_EQ(2, lru_replacer.Size()) << "Check your return value for LRUKReplacer::Size";
  lru_replacer.Remove(2);
  ASSERT_EQ(1, lru_replacer.Size()) << "Check your return value for LRUKReplacer::Size";
}

TEST(LRUKReplacerTest, Size5) {
  // Victiming a page should decrement its size
  LRUKReplacer lru_replacer(10, 3);
  int result;
  // 1 has three access histories, where as 2 only has two access histories
  lru_replacer.RecordAccess(1);
  lru_replacer.RecordAccess(1);
  lru_replacer.RecordAccess(2);
  lru_replacer.RecordAccess(1);
  lru_replacer.SetEvictable(2, true);
  lru_replacer.SetEvictable(1, true);
  ASSERT_EQ(2, lru_replacer.Size()) << "Check your return value for LRUKReplacer::Size";

  ASSERT_EQ(true, lru_replacer.Evict(&result));
  ASSERT_EQ(2, result);
  ASSERT_EQ(1, lru_replacer.Size()) << "Check your return value for LRUKReplacer::Size";
  ASSERT_EQ(true, lru_replacer.Evict(&result));
  ASSERT_EQ(1, result);
  ASSERT_EQ(0, lru_replacer.Size()) << "Check your return value for LRUKReplacer::Size";
}

TEST(LRUKReplacerTest, Size6) {
  LRUKReplacer lru_replacer(10, 2);
  lru_replacer.RecordAccess(1);
  lru_replacer.SetEvictable(1, true);
  lru_replacer.RecordAccess(2);
  lru_replacer.SetEvictable(2, true);
  lru_replacer.RecordAccess(3);
  lru_replacer.SetEvictable(3, true);
  ASSERT_EQ(3, lru_replacer.Size());
  lru_replacer.Remove(1);
  ASSERT_EQ(2, lru_replacer.Size());

  lru_replacer.SetEvictable(1, true);
  lru_replacer.SetEvictable(2, true);
  lru_replacer.SetEvictable(3, true);
  lru_replacer.Remove(2);
  ASSERT_EQ(1, lru_replacer.Size());

  // Delete non existent page should do nothing
  lru_replacer.Remove(1);
  lru_replacer.Remove(4);
  ASSERT_EQ(1, lru_replacer.Size());
}
}  // namespace bustub
