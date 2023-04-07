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
#include <cstdlib>
#include <functional>
#include <list>
#include <utility>

#include "container/hash/extendible_hash_table.h"
#include "primer/p0_trie.h"
#include "storage/page/page.h"

auto ClearMsb(int num) -> int {
  if (num == 0) {
    return 0;
  }
  int mask = 0x7FFFFFFF; // 0111 1111 1111 1111 1111 1111 1111 1111
  while ((num & mask) != 0) {
    mask >>= 1;
  }
  return num & mask;
}

namespace bustub {

template <typename K, typename V>
ExtendibleHashTable<K, V>::ExtendibleHashTable(size_t bucket_size)
    : global_depth_(1), bucket_size_(bucket_size), num_buckets_(2) {
  GetTestFileContent();
//  dir_.assign(2, std::make_shared<Bucket>(Bucket(bucket_size, 1)));
  dir_ = {std::make_shared<Bucket>(Bucket(bucket_size, 1)), std::make_shared<Bucket>(Bucket(bucket_size, 1))};
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
  std::cout << key << " dir_index:" << dir_index << std::endl;
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

template <typename K, typename V>
void ExtendibleHashTable<K, V>::Insert(const K &key, const V &value) {
  std::scoped_lock<std::mutex> lock(latch_);
  const auto [key_bucket, key_dir_index] = FindBucket(key);

  if(!key_bucket->IsFull()){
    key_bucket->Insert(key, value);
    return;
  }

  if(global_depth_ > key_bucket->GetDepth()){
    key_bucket->IncrementDepth();
    std::unordered_map<std::size_t, std::list<std::pair<K, V>>> divided_bucket_map;
    for (auto [k, v] : key_bucket->GetItems()) {
      auto dir_index = IndexOf(k);
      divided_bucket_map[dir_index].push_back(std::pair<K, V>(k, v));
    }

    for (auto& m : divided_bucket_map) {
      auto dir_index = m.first;
      auto pair_list = m.second;
      auto new_bucket = std::make_shared<Bucket>(Bucket(bucket_size_, key_bucket->GetDepth()));
      new_bucket->SetList(std::move(pair_list));
      dir_[dir_index] = new_bucket;
    }

    key_bucket->Insert(key, value);
    return;
  }

  dir_.resize(dir_.size() * 2);
  global_depth_++;
  key_bucket->IncrementDepth();
  std::unordered_map<std::size_t, std::list<std::pair<K, V>>> divided_bucket_map;
  for (auto [k, v] : key_bucket->GetItems()) {
    auto dir_index = IndexOf(k);
    divided_bucket_map[dir_index].push_back(std::pair<K, V>(k, v));
  }

  for (auto& m : divided_bucket_map) {
    auto dir_index = m.first;
    auto pair_list = m.second;
    auto new_bucket = std::make_shared<Bucket>(Bucket(bucket_size_, key_bucket->GetDepth()));
    new_bucket->SetList(std::move(pair_list));
    dir_[dir_index] = new_bucket;
  }
  const auto [new_key_bucket, _] = FindBucket(key);
  new_key_bucket->Insert(key, value);

  for(int i = 0; i < global_depth_*2; i++){
    if(dir_[i]){
      continue;
    }
    int clear_msb = ClearMsb(i);
    if(dir_[clear_msb]){
        dir_[i] = dir_[clear_msb];
    }
  }
}

//===--------------------------------------------------------------------===//
// Bucket
//===--------------------------------------------------------------------===//
template <typename K, typename V>
ExtendibleHashTable<K, V>::Bucket::Bucket(size_t array_size, int depth) : size_(array_size), depth_(depth) {}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::Bucket::Find(const K &key, V &value) -> bool {
  for(auto& p : list_){
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
  auto it = std::remove_if(
      list_.begin(), list_.end(), [&key](const std::pair<K, V> &element) { return element.first == key; });
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

  auto it = std::find_if(list_.begin(), list_.end(),
                         [&key](const std::pair<K, V> &element) { return element.first == key; });
  // 找到了具有特定key的元素，更新其value值
  if (it != list_.end()) {
    it->second = value;
    return false;
  }
  // 未找到具有特定key的元素，插入新元素到std::list的末尾
  list_.emplace_back(key, value);
  return true;
}

template class ExtendibleHashTable<page_id_t, Page *>;
template class ExtendibleHashTable<Page *, std::list<Page *>::iterator>;
template class ExtendibleHashTable<int, int>;
// test purpose
template class ExtendibleHashTable<int, std::string>;
template class ExtendibleHashTable<int, std::list<int>::iterator>;

}  // namespace bustub
