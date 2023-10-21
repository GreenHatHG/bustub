//===----------------------------------------------------------------------===//
//
//                         CMU-DB Project (15-445/645)
//                         ***DO NO SHARE PUBLICLY***
//
// Identification: src/page/b_plus_tree_internal_page.cpp
//
// Copyright (c) 2018, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <iostream>
#include <sstream>

#include "common/exception.h"
#include "storage/page/b_plus_tree_internal_page.h"

namespace bustub {
/*****************************************************************************
 * HELPER METHODS AND UTILITIES
 *****************************************************************************/
/*
 * Init method after creating a new internal page
 * Including set page type, set current size, set page id, set parent id and set
 * max page size
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::Init(page_id_t page_id, page_id_t parent_id, int max_size) {
  SetPageType(IndexPageType::INTERNAL_PAGE);
  SetSize(0);
  SetPageId(page_id);
  SetParentPageId(parent_id);
  SetMaxSize(max_size);
}
/*
 * Helper method to get/set the key associated with input "index"(a.k.a
 * array offset)
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::KeyAt(int index) const -> KeyType { return array_[index].first; }

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::SetKeyAt(int index, const KeyType &key) { array_[index].first = key; }

INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::IndexAt(int index) const -> MappingType { return array_[index]; }

/*
 * Helper method to get the value associated with input "index"(a.k.a array
 * offset)
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::ValueAt(int index) const -> ValueType { return array_[index].second; }

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::SetIndex(const size_t idx, const MappingType &m) { array_[idx] = m; }

INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::UpperBound(const KeyType &key, const KeyComparator &comparator) -> int {
  auto pair_comparator = [&](const KeyType &val, const MappingType &pair) -> bool {
    return comparator(val, pair.first) < 0;
  };
  return std::upper_bound(array_ + 1, array_ + GetSize(), key, pair_comparator) - array_ - 1;
}

INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::RemoveEntry(const KeyType &key, const KeyComparator &comparator) -> bool {
  auto key_idx = UpperBound(key, comparator);
  if(key_idx == GetSize()){
    return false;
  }

  // ArrayIndexOutOfBoundsException ?
  for(int i = key_idx; i < GetSize(); i++){
    array_[key_idx] = array_[key_idx + 1];
  }

  IncreaseSize(-1);
}

INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::Insert(const KeyType &key, const ValueType &value, const KeyComparator &comparator)
    -> void {
  int be_inserted_idx = UpperBound(key, comparator) + 1;
  for (int i = GetSize(); i > be_inserted_idx; i--) {
    array_[i] = array_[i - 1];
  }
  array_[be_inserted_idx] = {key, value};
  IncreaseSize(1);
}

INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::InsertAtBack(const KeyType &key, const ValueType &value) -> void {
  array_[GetSize()] = {key, value};
  IncreaseSize(1);
}

INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::InsertAtSecond(const KeyType &key, const ValueType &value) -> void {
  for(int i = GetSize(); i >= 2; i--){
    array_[i] = array_[i-1];
  }
  array_[1] = {key, value};
  IncreaseSize(1);
}

// 用于删除操作时。
// 传入子节点的page_id，找到该子节点在父节点的位置，然后返回该位置的前一个位置，也就是子节点的左兄弟节点。
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::GetLeftSiblingPageIdx(const ValueType &child_page_id, bool& ok, KeyType& parentKey, int& parent_idx) -> int{
  for(int i = 0; i < GetSize(); i++){
    if(ValueAt(i) == child_page_id){
      if(i == 0){
        ok = false;
        parentKey = KeyAt(1);
        parent_idx = 1;
        return 1;
      }

      ok = true;
      parentKey = KeyAt(i);
      parent_idx = i;
      return i - 1;
    }
  }
}
//
// INDEX_TEMPLATE_ARGUMENTS
// auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::FindSmallestNumber(const KeyType &key, const KeyComparator &comparator) ->
// ValueType{
//    auto size = GetSize();
//    for(int i = 0; i < size; i++){
//      /*comparator(a,b) = 0, if a = b
//        comparator(a,b) > 0, if a > b
//        comparator(a,b) < 0, if a < b
//       */
//      if(comparator(key, array_[i].first) == -1){
//        return array_[i].second;
//      }
//    }
//    return array_[size - 1].second;
//}

// valuetype for internalNode should be page id_t
template class BPlusTreeInternalPage<GenericKey<4>, page_id_t, GenericComparator<4>>;
template class BPlusTreeInternalPage<GenericKey<8>, page_id_t, GenericComparator<8>>;
template class BPlusTreeInternalPage<GenericKey<16>, page_id_t, GenericComparator<16>>;
template class BPlusTreeInternalPage<GenericKey<32>, page_id_t, GenericComparator<32>>;
template class BPlusTreeInternalPage<GenericKey<64>, page_id_t, GenericComparator<64>>;
}  // namespace bustub
