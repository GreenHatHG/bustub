//===----------------------------------------------------------------------===//
//
//                         CMU-DB Project (15-445/645)
//                         ***DO NO SHARE PUBLICLY***
//
// Identification: src/include/index/index_iterator.h
//
// Copyright (c) 2018, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
/**
 * index_iterator.h
 * For range scan of b+ tree
 */
#pragma once
#include "storage/page/b_plus_tree_leaf_page.h"

namespace bustub {

#define INDEXITERATOR_TYPE IndexIterator<KeyType, ValueType, KeyComparator>

INDEX_TEMPLATE_ARGUMENTS
class IndexIterator {
  using LeafPage = BPlusTreeLeafPage<KeyType, ValueType, KeyComparator>;

 public:
  // you may define your own constructor based on your member variables
  explicit IndexIterator(LeafPage *current_page, BufferPoolManager *buffer_pool_manager, int arr_idx = 0);
  ~IndexIterator();  // NOLINT

  auto IsEnd() -> bool;

  auto operator*() -> const MappingType &;

  auto operator++() -> IndexIterator &;

  auto operator==(const IndexIterator &itr) const -> bool {
    return itr.current_page_ == current_page_ && itr.arr_idx_ == arr_idx_;
  }

  auto operator!=(const IndexIterator &itr) const -> bool {
    return !(itr.current_page_ == current_page_ && itr.arr_idx_ == arr_idx_);
  }

 private:
  // add your own private member variables here
  LeafPage *current_page_{};
  int arr_idx_{0};
  BufferPoolManager *buffer_pool_manager_;
};

}  // namespace bustub
