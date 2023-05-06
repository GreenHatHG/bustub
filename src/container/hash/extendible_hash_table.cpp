//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// extendible_hash_table.cpp
//
// Identification: src/container/hash/extendible_hash_table.cpp
//
// Copyright (c) 2022, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "container/hash/extendible_hash_table.h"
#include "storage/page/page.h"

auto ClearLeftMostBit(int num) -> int {
  if (num < 0) {
    return num;
  }
  return num & ~(1 << (sizeof(num) * 8 - __builtin_clz(num) - 1));
}

namespace bustub {

template <typename K, typename V>
ExtendibleHashTable<K, V>::ExtendibleHashTable(size_t bucket_size)
    : global_depth_(0), bucket_size_(bucket_size), num_buckets_(1) {
  //  dir_.assign(2, std::make_shared<Bucket>(Bucket(bucket_size, 1)));
  std::cout << "[Init] bucket_size:" << bucket_size << std::endl;
  dir_ = {std::make_shared<Bucket>(bucket_size, 0)};
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::IndexOf(const K &key) -> size_t {
  int mask = (1 << global_depth_) - 1;
  return std::hash<K>()(key) & mask;
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::GetGlobalDepth() const -> int {
  std::scoped_lock<std::mutex> lock(latch_);
  return GetGlobalDepthInternal();
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::GetGlobalDepthInternal() const -> int {
  return global_depth_;
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::GetLocalDepth(int dir_index) const -> int {
  std::scoped_lock<std::mutex> lock(latch_);
  return GetLocalDepthInternal(dir_index);
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::GetLocalDepthInternal(int dir_index) const -> int {
  return dir_[dir_index]->GetDepth();
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::GetNumBuckets() const -> int {
  std::scoped_lock<std::mutex> lock(latch_);
  return GetNumBucketsInternal();
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::GetNumBucketsInternal() const -> int {
  return num_buckets_;
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::FindBucket(const K &key) -> std::tuple<std::shared_ptr<Bucket>, size_t> {
  auto dir_index = IndexOf(key);
  std::cout << "[FindBucket] key:" << key << ", dir_index:" << dir_index << std::endl;
  std::shared_ptr<Bucket> bucket = dir_[dir_index];
  return std::make_tuple(bucket, dir_index);
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::Find(const K &key, V &value) -> bool {
  std::scoped_lock<std::mutex> lock(latch_);
  auto [bucket, _] = FindBucket(key);
  return bucket->Find(key, value);
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::Remove(const K &key) -> bool {
  std::scoped_lock<std::mutex> lock(latch_);
  auto [bucket, _] = FindBucket(key);
  return bucket->Remove(key);
}

auto operator<<(std::ostream &os, const std::list<Page *>::iterator &it) -> std::ostream & {
  os << *it;
  return os;
}

auto operator<<(std::ostream &os, const std::list<int>::iterator &it) -> std::ostream & {
  os << *it;
  return os;
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::RedistributeBucket(std::shared_ptr<Bucket> bucket) -> void {
  const int bucket_old_depth = bucket->GetDepth();
  const size_t bucket_old_mask = (1 << bucket_old_depth) - 1;
  const size_t bucket_old_last_depth_bits = std::hash<K>()(bucket->GetItems().begin()->first) & bucket_old_mask;

  if (global_depth_ == bucket->GetDepth()) {
    global_depth_++;
    const size_t dir_size = dir_.size();
    for (size_t i = 0; i < dir_size; i++) {
      dir_.emplace_back(dir_[i]);
    }
  }

  bucket->IncrementDepth();
  auto new_bucket = std::make_shared<Bucket>(bucket_size_, bucket->GetDepth());
  num_buckets_++;

  int check_bit_set_mask = 1 << (bucket->GetDepth() - 1);
  for (auto it = bucket->GetItems().begin(); it != bucket->GetItems().end();) {
    bool is_bit_set = (std::hash<K>()(it->first) & check_bit_set_mask) != 0;
    if (is_bit_set) {
      new_bucket->Insert(it->first, it->second);
      it = bucket->GetItems().erase(it);
    } else {
      it++;
    }
  }

  for (size_t i = 0; i < dir_.size(); i++) {
    bool is_prev_full_bucket_ptr = (i & bucket_old_mask) == bucket_old_last_depth_bits;
    bool is_bit_set = (i & check_bit_set_mask) != 0;
    if (is_prev_full_bucket_ptr && is_bit_set) {
      dir_[i] = new_bucket;
    }
  }
}

template <typename K, typename V>
void ExtendibleHashTable<K, V>::Insert(const K &key, const V &value) {
  std::scoped_lock<std::mutex> lock(latch_);
  std::cout << "===========[Insert]"
            << "key: " << key << " ,value:" << value << std::endl;
  auto [key_bucket, key_dir_index] = FindBucket(key);

  while (!key_bucket->Insert(key, value)) {
    std::cout << "key_bucket is full" << std::endl;
    std::cout << "global_depth: " << global_depth_ << "local depth: " << key_bucket->GetDepth() << std::endl;
    std::cout << "before RedistributeBucket" << std::endl;
    PrintDir();
    RedistributeBucket(dir_[key_dir_index]);
    std::cout << "after RedistributeBucket" << std::endl;
    PrintDir();
    std::tie(key_bucket, key_dir_index) = FindBucket(key);
  }
  PrintDir();
}

//===--------------------------------------------------------------------===//
// Bucket
//===--------------------------------------------------------------------===//
template <typename K, typename V>
ExtendibleHashTable<K, V>::Bucket::Bucket(size_t array_size, int depth) : size_{array_size}, depth_{depth} {}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::Bucket::Find(const K &key, V &value) -> bool {
  for (auto &p : list_) {
    if (p.first == key) {
      value = p.second;
      return true;
    }
  }
  return false;
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::Bucket::Remove(const K &key) -> bool {
  // 当找到元素时，std::remove_if函数将把该元素移到列表的末尾，并返回指向新的结尾的迭代器。
  // 最后，函数使用std::list的成员函数erase来删除移动到列表末尾的元素。这将删除所有具有给定key值的元素
  // 由于std::remove_if函数并没有真正删除元素，因此我们需要使用std::list的成员函数erase来真正删除元素。
  // 这种删除方法可以避免在删除元素时破坏std::list的迭代器，从而确保程序的正确性
  auto it = std::remove_if(list_.begin(), list_.end(),
                           [&key](const std::pair<K, V> &element) { return element.first == key; });
  if (it != list_.end()) {
    list_.erase(it, list_.end());
    return true;
  }
  return false;
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::Bucket::Insert(const K &key, const V &value) -> bool {
  if (IsFull()) {
    return false;
  }

  auto it =
      std::find_if(list_.begin(), list_.end(), [&key](const std::pair<K, V> &element) { return element.first == key; });
  // 找到了具有特定key的元素，更新其value值
  if (it != list_.end()) {
    it->second = value;
  } else {
    // 未找到具有特定key的元素，插入新元素到std::list的末尾
    list_.emplace_back(key, value);
  }
  return true;
}

template class ExtendibleHashTable<page_id_t, Page *>;
template class ExtendibleHashTable<Page *, std::list<Page *>::iterator>;
template class ExtendibleHashTable<int, int>;
// test purpose
template class ExtendibleHashTable<int, std::string>;
template class ExtendibleHashTable<int, std::list<int>::iterator>;

}  // namespace bustub
