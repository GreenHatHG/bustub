/**
 * index_iterator.cpp
 */
#include <cassert>

#include "storage/index/index_iterator.h"

namespace bustub {

/*
 * NOTE: you can change the destructor/constructor method here
 * set your own input parameters
 */
INDEX_TEMPLATE_ARGUMENTS
INDEXITERATOR_TYPE::IndexIterator(LeafPage *current_page, BufferPoolManager *buffer_pool_manager, int arr_idx)
    : current_page_(current_page), arr_idx_(arr_idx), buffer_pool_manager_(buffer_pool_manager) {}

INDEX_TEMPLATE_ARGUMENTS
INDEXITERATOR_TYPE::~IndexIterator() = default;  // NOLINT

INDEX_TEMPLATE_ARGUMENTS
auto INDEXITERATOR_TYPE::IsEnd() -> bool { return current_page_ == nullptr; }

INDEX_TEMPLATE_ARGUMENTS
auto INDEXITERATOR_TYPE::operator*() -> const MappingType & { return current_page_->IndexAt(arr_idx_); }

INDEX_TEMPLATE_ARGUMENTS
auto INDEXITERATOR_TYPE::operator++() -> INDEXITERATOR_TYPE & {
  if (arr_idx_ == current_page_->GetSize()) {
    buffer_pool_manager_->UnpinPage(current_page_->GetPageId(), false);
    auto next_page_id = current_page_->GetNextPageId();
    current_page_ = reinterpret_cast<LeafPage *>(buffer_pool_manager_->FetchPage(next_page_id)->GetData());
    arr_idx_ = 0;
  } else {
    arr_idx_++;
  }
  return *this;
}

template class IndexIterator<GenericKey<4>, RID, GenericComparator<4>>;

template class IndexIterator<GenericKey<8>, RID, GenericComparator<8>>;

template class IndexIterator<GenericKey<16>, RID, GenericComparator<16>>;

template class IndexIterator<GenericKey<32>, RID, GenericComparator<32>>;

template class IndexIterator<GenericKey<64>, RID, GenericComparator<64>>;

}  // namespace bustub
