#include <string>

#include "common/exception.h"
#include "common/logger.h"
#include "common/rid.h"
#include "storage/index/b_plus_tree.h"
#include "storage/page/header_page.h"

namespace bustub {
INDEX_TEMPLATE_ARGUMENTS
BPLUSTREE_TYPE::BPlusTree(std::string name, BufferPoolManager *buffer_pool_manager, const KeyComparator &comparator,
                          int leaf_max_size, int internal_max_size)
    : index_name_(std::move(name)),
      root_page_id_(INVALID_PAGE_ID),
      buffer_pool_manager_(buffer_pool_manager),
      comparator_(comparator),
      leaf_max_size_(leaf_max_size),
      internal_max_size_(internal_max_size) {
  std::cout << "[BPLUSTREE_TYPE::BPlusTree] - "
            << "leaf_max_size:" << leaf_max_size << ", internal_max_size:" << internal_max_size << std::endl;
}

/*
 * Helper function to decide whether current b+tree is empty
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::IsEmpty() const -> bool { return root_page_id_ == INVALID_PAGE_ID; }
/*****************************************************************************
 * SEARCH
 *****************************************************************************/
/*
 * Return the only value that associated with input key
 * This method is used for point query
 * @return : true means key exists
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::GetValue(const KeyType &key, std::vector<ValueType> *result, Transaction *transaction) -> bool {
  auto leaf_node = ReachLeafNode(key);
  for (int i = 0; i < leaf_node->GetSize(); i++) {
    if (comparator_(leaf_node->KeyAt(i), key) == 0) {
      result->push_back(leaf_node->ValueAt(i));
      break;
    }
  }
  buffer_pool_manager_->UnpinPage(leaf_node->GetPageId(), false);
  return !result->empty();
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::ReachLeafNode(const KeyType &key) -> LeafPage * {
  auto page = buffer_pool_manager_->FetchPage(root_page_id_);
  auto current = reinterpret_cast<BPlusTreePage *>(page->GetData());

  while (!current->IsLeafPage()) {
    auto *internal_page = reinterpret_cast<InternalPage *>(current);
    int idx = internal_page->UpperBound(key, comparator_);
    auto next_page_id = internal_page->ValueAt(idx);
    buffer_pool_manager_->UnpinPage(current->GetPageId(), false);

    page = buffer_pool_manager_->FetchPage(next_page_id);
    current = reinterpret_cast<BPlusTreePage *>(page->GetData());
  }
  return reinterpret_cast<LeafPage *>(current);
}

/*****************************************************************************
 * INSERTION
 *****************************************************************************/
/*
 * Insert constant key & value pair into b+ tree
 * if current tree is empty, start new tree, update root page id and insert
 * entry, otherwise insert into leaf page.
 * @return: since we only support unique key, if user try to insert duplicate
 * keys return false, otherwise return true.
 */

INDEX_TEMPLATE_ARGUMENTS
template <typename NodeType>
auto BPLUSTREE_TYPE::NewNode() -> NodeType * {
  page_id_t new_page_id{INVALID_PAGE_ID};
  auto *new_node = reinterpret_cast<NodeType *>(buffer_pool_manager_->NewPage(&new_page_id));
  if (std::is_same<NodeType, LeafPage>::value) {
    new_node->Init(new_page_id, INVALID_PAGE_ID, leaf_max_size_);
  } else {
    new_node->Init(new_page_id, INVALID_PAGE_ID, internal_max_size_);
  }
  return new_node;
}

INDEX_TEMPLATE_ARGUMENTS
template <typename NodeType>
auto BPLUSTREE_TYPE::CopyToMemory(NodeType *node) -> NodeType * {
  auto *new_node = NewNode<NodeType>();
  for (int i = 0; i < node->GetSize(); i++) {
    new_node->SetIndex(i, node->IndexAt(i));
  }
  new_node->IncreaseSize(node->GetSize());
  return new_node;
}

INDEX_TEMPLATE_ARGUMENTS
template <typename NodeType>
auto BPLUSTREE_TYPE::SplitNodes(NodeType &n, NodeType &n_new) -> void {
  int mid = (n_new->GetSize() + 1) / 2;
  int n_size = mid;
  int n_new_size = n_new->GetSize() - mid;

  n->SetSize(n_size);
  for (int i = 0; i < n_size; i++) {
    n->SetIndex(i, n_new->IndexAt(i));
  }

  n_new->SetSize(n_new_size);
  int n_new_begin = 0;
  if (std::is_same<NodeType, InternalPage>::value) {
    n_new_begin = 1;
  }
  for (int i = n_new_begin, j = n_size; i < n_new_size; i++, j++) {
    n_new->SetIndex(i, n_new->IndexAt(j));
  }
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::InsertInParent(BPlusTreePage *n, const KeyType &k_new, BPlusTreePage *n_new) -> void {
  if (n->IsRootPage()) {
    auto *root_new = NewNode<InternalPage>();

    root_page_id_ = root_new->GetPageId();
    UpdateRootPageId(0);

    root_new->InsertAtBack(KeyType{}, n->GetPageId());
    root_new->InsertAtBack(k_new, n_new->GetPageId());

    n->SetParentPageId(root_new->GetPageId());
    n_new->SetParentPageId(root_new->GetPageId());

    buffer_pool_manager_->UnpinPage(root_new->GetPageId(), true);
    return;
  }

  auto parent_id = n->GetParentPageId();
  auto *parent = reinterpret_cast<InternalPage *>(buffer_pool_manager_->FetchPage(parent_id)->GetData());
  // 对于internal node，插入之前的size等于max_size需要拆分
  // 所以这里插入前不拆分的极端情况是max_size-1
  if (parent->GetSize() < parent->GetMaxSize()) {
    parent->Insert(k_new, n_new->GetPageId(), comparator_);
    n_new->SetParentPageId(parent_id);
    buffer_pool_manager_->UnpinPage(parent_id, true);
    return;
  }

  // 这里需要注意此时internal node size已经等于max size，不能插入了
  InternalPage *t = CopyToMemory(parent);
  t->Insert(k_new, n_new->GetPageId(), comparator_);
  SplitNodes(parent, t);
  n->SetParentPageId(t->GetPageId());
  n_new->SetParentPageId(t->GetPageId());
  buffer_pool_manager_->UnpinPage(t->GetPageId(), true);
  buffer_pool_manager_->UnpinPage(parent->GetPageId(), true);
  InsertInParent(parent, t->KeyAt(0), t);
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::NewRoot(const KeyType &key, const ValueType &value) -> void {
  auto *r = NewNode<LeafPage>();
  r->InsertAtBack(key, value);
  root_page_id_ = r->GetPageId();
  UpdateRootPageId(1);
  buffer_pool_manager_->UnpinPage(r->GetPageId(), true);
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Insert(const KeyType &key, const ValueType &value, Transaction *transaction) -> bool {
  std::cout << "[BPLUSTREE_TYPE::Insert] - "
            << "key: " << key << ", value: " << value << std::endl;
  if (IsEmpty()) {
    NewRoot(key, value);
    return true;
  }

  auto leaf = ReachLeafNode(key);
  if (leaf->ExistsKey(key, comparator_)) {
    buffer_pool_manager_->UnpinPage(leaf->GetPageId(), false);
    return false;
  }

  // leaf node拆分的条件是插入后size等于max_size，这里是插入成功但是不拆分，所以需要满足小于max_size-1
  // 极端情况下等于max_size-1，插入后就需要拆分了
  if (leaf->GetSize() < leaf->GetMaxSize() - 1) {
    leaf->Insert(key, value, comparator_);
    buffer_pool_manager_->UnpinPage(leaf->GetPageId(), true);
    return true;
  }

  // 这里leaf node大小已经是max_size-1，但是还能插入一个，所以直接申请一个page没问题
  auto leaf_new = CopyToMemory(leaf);
  leaf_new->Insert(key, value, comparator_);

  SplitNodes(leaf, leaf_new);
  leaf_new->SetNextPageId(leaf->GetNextPageId());
  leaf->SetNextPageId(leaf_new->GetPageId());

  InsertInParent(leaf, leaf_new->KeyAt(0), leaf_new);
  buffer_pool_manager_->UnpinPage(leaf->GetPageId(), true);
  buffer_pool_manager_->UnpinPage(leaf_new->GetPageId(), true);
  return true;
}

/*****************************************************************************
 * REMOVE
 *****************************************************************************/
/*
 * Delete key & value pair associated with input key
 * If current tree is empty, return immdiately.
 * If not, User needs to first find the right leaf page as deletion target, then
 * delete entry from leaf page. Remember to deal with redistribute or merge if
 * necessary.
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::Remove(const KeyType &key, Transaction *transaction) {
  if(IsEmpty()){
    return;
  }

  auto* leaf = ReachLeafNode(key);
  DeleteEntry(leaf, key);
}

INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::DeleteEntry(BPlusTreePage* current, const KeyType &key){
  if(current->IsLeafPage()){
    auto current_leaf = reinterpret_cast<LeafPage *>(current);
    if(!current_leaf->RemoveEntry(key, comparator_)){
      return;
    }
  }else{
    auto current_internal = reinterpret_cast<InternalPage *>(current);
    if(!current_internal->RemoveEntry(key, comparator_)){
      return;
    }
  }

  if(current->IsRootPage()){
    if(current->IsLeafPage() && current->GetSize() == 0){
      root_page_id_ = INVALID_PAGE_ID;
    }
    if(!current->IsLeafPage() && current->GetSize() == 1){
      auto current_internal = reinterpret_cast<InternalPage *>(current);
      root_page_id_ = current_internal->ValueAt(0);
    }
    UpdateRootPageId(0);
    return;
  }

  if(current->GetSize() >= current->GetMinSize()){
    return;
  }

  auto parent_id = current->GetParentPageId();
  auto *parent = reinterpret_cast<InternalPage *>(buffer_pool_manager_->FetchPage(parent_id)->GetData());

  bool exist_left_sibling;
  KeyType parent_key{};
  int parent_idx;
  auto left_sibling_page_idx = parent->GetLeftSiblingPageIdx(current->GetPageId(), exist_left_sibling, parent_key, parent_idx);
  auto *leaf_sibling_page = reinterpret_cast<InternalPage *>(buffer_pool_manager_->FetchPage(left_sibling_page_idx)->GetData());

  if(leaf_sibling_page->GetSize() + current->GetSize() <= current->GetMaxSize()){
    CoalesceNodes(exist_left_sibling, current, leaf_sibling_page, parent_key);
    DeleteEntry(parent, parent_key);
  }else{
    RedistributeNodes(exist_left_sibling, current, leaf_sibling_page,parent, parent_key, parent_idx);
  }

  buffer_pool_manager_->UnpinPage(left_sibling_page_idx, true);
  buffer_pool_manager_->UnpinPage(parent_id, true);
  buffer_pool_manager_->UnpinPage(current->GetPageId(), true);
}

INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::CoalesceNodes(bool exist_left_sibling, BPlusTreePage *n, BPlusTreePage *sibling_page, const KeyType &parent_key){
  // 默认是合并到左边兄弟节点，现在没有左边兄弟节点，则需要合入到右边兄弟节点，所以这里需要交换一下变量
  if(!exist_left_sibling){
    std::swap(n, sibling_page);
  }

  if(!n->IsLeafPage()){
    auto *sibling_internal = reinterpret_cast<InternalPage* >(sibling_page);
    auto *n_internal = reinterpret_cast<InternalPage* >(n);
    int begin = sibling_internal->GetSize();
    sibling_internal->InsertAtBack(parent_key, n_internal->ValueAt(0));
    for(int i = 1; i < n_internal->GetSize(); i++){
      sibling_internal->InsertAtBack(n_internal->KeyAt(i), n_internal->ValueAt(i));
    }
    for(int i = begin; i < n_internal->GetSize(); i++){
      auto child_page_id = sibling_internal->ValueAt(i);
      auto child_page = buffer_pool_manager_->FetchPage(child_page_id);
      auto child = reinterpret_cast<InternalPage* >(child_page->GetData());
      child->SetParentPageId(sibling_internal->GetPageId());
      buffer_pool_manager_->UnpinPage(child_page_id, true);
    }
  }else{
    auto *leaf = reinterpret_cast<LeafPage *>(n);
    auto *sibling_leaf = reinterpret_cast<LeafPage *>(sibling_page);
    for(int i = 0; i < leaf->GetSize(); i++){
      sibling_leaf->InsertAtBack(leaf->KeyAt(i), leaf->ValueAt(i));
    }
    sibling_leaf->SetNextPageId(leaf->GetNextPageId());
  }
  buffer_pool_manager_->UnpinPage(sibling_page->GetPageId(), true);
  buffer_pool_manager_->DeletePage(n->GetPageId());
}

INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::RedistributeNodes(bool exist_left_sibling, BPlusTreePage *n, BPlusTreePage *sibling_page,
                                       InternalPage* parent_page, KeyType parent_key, int parent_idx){
  KeyType key{};
  if(n->IsLeafPage()){
    auto *n_leaf = reinterpret_cast<LeafPage *>(n);
    auto *sibling_leaf = reinterpret_cast<LeafPage *>(sibling_page);
    // 这里不能直接交换变量，比如 (1) (2 3 4)，交换变量后redistribute就变成了(1 4)(2 3)，不符合有序
    if(exist_left_sibling){
      auto last_key = sibling_leaf->KeyAt(sibling_leaf->GetSize());
      auto last_value = sibling_leaf->ValueAt(sibling_leaf->GetSize());
      n_leaf->InsertAtSecond(last_key, last_value);
      sibling_leaf->RemoveEntry(last_key, comparator_);
      key = last_key;
    }else{
      auto first_key = sibling_leaf->KeyAt(1);
      auto first_value = sibling_leaf->ValueAt(1);
      n_leaf->InsertAtSecond(first_key, first_value);
      sibling_leaf->RemoveEntry(first_key, comparator_);
      key = first_key;
    }
    parent_page->SetKeyAt(parent_idx, key);
    //todo need?
    buffer_pool_manager_->UnpinPage(n->GetPageId(),true);
    buffer_pool_manager_->UnpinPage(sibling_page->GetPageId(),true);
  }else{
    auto n_internal = reinterpret_cast<InternalPage* >(n);
    auto sibling_internal = reinterpret_cast<InternalPage* >(sibling_page);
    page_id_t child_page_id;
    if(exist_left_sibling){
      auto last_key = sibling_internal->KeyAt(sibling_page->GetSize());
      auto last_value = sibling_internal->ValueAt(sibling_internal->GetSize());
      n_internal->InsertAtSecond(parent_key, last_value);
      sibling_internal->RemoveEntry(last_key, comparator_);
      key = last_key;
      child_page_id = last_value;
    }else{
      // todo note!!!!
      auto first_key = sibling_internal->KeyAt(1);
      auto first_value = sibling_internal->ValueAt(1);
      n_internal->InsertAtSecond(parent_key, first_value);
      sibling_internal->RemoveEntry(first_key, comparator_);
      key = first_key;
      child_page_id = first_value;
    }
    parent_page->SetKeyAt(parent_idx, key);

    auto child_page = buffer_pool_manager_->FetchPage(child_page_id);
    auto child_node = reinterpret_cast<BPlusTreePage *>(child_page->GetData());
    if (child_node->IsLeafPage()) {
      auto leaf_child_node = reinterpret_cast<LeafPage *>(child_page->GetData());
      leaf_child_node->SetParentPageId(n->GetPageId());
    }else{
      auto internal_child_node = reinterpret_cast<InternalPage *>(child_page->GetData());
      internal_child_node->SetParentPageId(n->GetPageId());
    }
    buffer_pool_manager_->UnpinPage(child_page_id, true);
  }
}
/*****************************************************************************
 * INDEX ITERATOR
 *****************************************************************************/
/*
 * Input parameter is void, find the leaftmost leaf page first, then construct
 * index iterator
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Begin() -> INDEXITERATOR_TYPE { return INDEXITERATOR_TYPE(); }

/*
 * Input parameter is low key, find the leaf page that contains the input key
 * first, then construct index iterator
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Begin(const KeyType &key) -> INDEXITERATOR_TYPE { return INDEXITERATOR_TYPE(); }

/*
 * Input parameter is void, construct an index iterator representing the end
 * of the key/value pair in the leaf node
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::End() -> INDEXITERATOR_TYPE { return INDEXITERATOR_TYPE(); }

/**
 * @return Page id of the root of this tree
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::GetRootPageId() -> page_id_t { return root_page_id_; }

/*****************************************************************************
 * UTILITIES AND DEBUG
 *****************************************************************************/
/*
 * Update/Insert root page id in header page(where page_id = 0, header_page is
 * defined under include/page/header_page.h)
 * Call this method everytime root page id is changed.
 * @parameter: insert_record      defualt value is false. When set to true,
 * insert a record <index_name, root_page_id> into header page instead of
 * updating it.
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::UpdateRootPageId(int insert_record) {
  auto *header_page = static_cast<HeaderPage *>(buffer_pool_manager_->FetchPage(HEADER_PAGE_ID));
  if (insert_record != 0) {
    // create a new record<index_name + root_page_id> in header_page
    header_page->InsertRecord(index_name_, root_page_id_);
  } else {
    // update root_page_id in header_page
    header_page->UpdateRecord(index_name_, root_page_id_);
  }
  buffer_pool_manager_->UnpinPage(HEADER_PAGE_ID, true);
}

/*
 * This method is used for test only
 * Read data from file and insert one by one
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::InsertFromFile(const std::string &file_name, Transaction *transaction) {
  int64_t key;
  std::ifstream input(file_name);
  while (input) {
    input >> key;

    KeyType index_key;
    index_key.SetFromInteger(key);
    RID rid(key);
    Insert(index_key, rid, transaction);
  }
}
/*
 * This method is used for test only
 * Read data from file and remove one by one
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::RemoveFromFile(const std::string &file_name, Transaction *transaction) {
  int64_t key;
  std::ifstream input(file_name);
  while (input) {
    input >> key;
    KeyType index_key;
    index_key.SetFromInteger(key);
    Remove(index_key, transaction);
  }
}

/**
 * This method is used for debug only, You don't need to modify
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::Draw(BufferPoolManager *bpm, const std::string &outf) {
  if (IsEmpty()) {
    LOG_WARN("Draw an empty tree");
    return;
  }
  std::ofstream out(outf);
  out << "digraph G {" << std::endl;
  ToGraph(reinterpret_cast<BPlusTreePage *>(bpm->FetchPage(root_page_id_)->GetData()), bpm, out);
  out << "}" << std::endl;
  out.flush();
  out.close();
}

/**
 * This method is used for debug only, You don't need to modify
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::Print(BufferPoolManager *bpm) {
  if (IsEmpty()) {
    LOG_WARN("Print an empty tree");
    return;
  }
  ToString(reinterpret_cast<BPlusTreePage *>(bpm->FetchPage(root_page_id_)->GetData()), bpm);
}

/**
 * This method is used for debug only, You don't need to modify
 * @tparam KeyType
 * @tparam ValueType
 * @tparam KeyComparator
 * @param page
 * @param bpm
 * @param out
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::ToGraph(BPlusTreePage *page, BufferPoolManager *bpm, std::ofstream &out) const {
  std::string leaf_prefix("LEAF_");
  std::string internal_prefix("INT_");
  if (page->IsLeafPage()) {
    auto *leaf = reinterpret_cast<LeafPage *>(page);
    // Print node name
    out << leaf_prefix << leaf->GetPageId();
    // Print node properties
    out << "[shape=plain color=green ";
    // Print data of the node
    out << "label=<<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\">\n";
    // Print data
    out << "<TR><TD COLSPAN=\"" << leaf->GetSize() << "\">P=" << leaf->GetPageId() << "</TD></TR>\n";
    out << "<TR><TD COLSPAN=\"" << leaf->GetSize() << "\">"
        << "max_size=" << leaf->GetMaxSize() << ",min_size=" << leaf->GetMinSize() << ",size=" << leaf->GetSize()
        << "</TD></TR>\n";
    out << "<TR>";
    for (int i = 0; i < leaf->GetSize(); i++) {
      out << "<TD>" << leaf->KeyAt(i) << "</TD>\n";
    }
    out << "</TR>";
    // Print table end
    out << "</TABLE>>];\n";
    // Print Leaf node link if there is a next page
    if (leaf->GetNextPageId() != INVALID_PAGE_ID) {
      out << leaf_prefix << leaf->GetPageId() << " -> " << leaf_prefix << leaf->GetNextPageId() << ";\n";
      out << "{rank=same " << leaf_prefix << leaf->GetPageId() << " " << leaf_prefix << leaf->GetNextPageId() << "};\n";
    }

    // Print parent links if there is a parent
    if (leaf->GetParentPageId() != INVALID_PAGE_ID) {
      out << internal_prefix << leaf->GetParentPageId() << ":p" << leaf->GetPageId() << " -> " << leaf_prefix
          << leaf->GetPageId() << ";\n";
    }
  } else {
    auto *inner = reinterpret_cast<InternalPage *>(page);
    // Print node name
    out << internal_prefix << inner->GetPageId();
    // Print node properties
    out << "[shape=plain color=pink ";  // why not?
    // Print data of the node
    out << "label=<<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\">\n";
    // Print data
    out << "<TR><TD COLSPAN=\"" << inner->GetSize() << "\">P=" << inner->GetPageId() << "</TD></TR>\n";
    out << "<TR><TD COLSPAN=\"" << inner->GetSize() << "\">"
        << "max_size=" << inner->GetMaxSize() << ",min_size=" << inner->GetMinSize() << ",size=" << inner->GetSize()
        << "</TD></TR>\n";
    out << "<TR>";
    for (int i = 0; i < inner->GetSize(); i++) {
      out << "<TD PORT=\"p" << inner->ValueAt(i) << "\">";
      if (i > 0) {
        out << inner->KeyAt(i);
      } else {
        out << " ";
      }
      out << "</TD>\n";
    }
    out << "</TR>";
    // Print table end
    out << "</TABLE>>];\n";
    // Print Parent link
    if (inner->GetParentPageId() != INVALID_PAGE_ID) {
      out << internal_prefix << inner->GetParentPageId() << ":p" << inner->GetPageId() << " -> " << internal_prefix
          << inner->GetPageId() << ";\n";
    }
    // Print leaves
    for (int i = 0; i < inner->GetSize(); i++) {
      auto child_page = reinterpret_cast<BPlusTreePage *>(bpm->FetchPage(inner->ValueAt(i))->GetData());
      ToGraph(child_page, bpm, out);
      if (i > 0) {
        auto sibling_page = reinterpret_cast<BPlusTreePage *>(bpm->FetchPage(inner->ValueAt(i - 1))->GetData());
        if (!sibling_page->IsLeafPage() && !child_page->IsLeafPage()) {
          out << "{rank=same " << internal_prefix << sibling_page->GetPageId() << " " << internal_prefix
              << child_page->GetPageId() << "};\n";
        }
        bpm->UnpinPage(sibling_page->GetPageId(), false);
      }
    }
  }
  bpm->UnpinPage(page->GetPageId(), false);
}

/**
 * This function is for debug only, you don't need to modify
 * @tparam KeyType
 * @tparam ValueType
 * @tparam KeyComparator
 * @param page
 * @param bpm
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::ToString(BPlusTreePage *page, BufferPoolManager *bpm) const {
  if (page->IsLeafPage()) {
    auto *leaf = reinterpret_cast<LeafPage *>(page);
    std::cout << "Leaf Page: " << leaf->GetPageId() << " parent: " << leaf->GetParentPageId()
              << " next: " << leaf->GetNextPageId() << std::endl;
    for (int i = 0; i < leaf->GetSize(); i++) {
      std::cout << leaf->KeyAt(i) << ",";
    }
    std::cout << std::endl;
    std::cout << std::endl;
  } else {
    auto *internal = reinterpret_cast<InternalPage *>(page);
    std::cout << "Internal Page: " << internal->GetPageId() << " parent: " << internal->GetParentPageId() << std::endl;
    for (int i = 0; i < internal->GetSize(); i++) {
      std::cout << internal->KeyAt(i) << ": " << internal->ValueAt(i) << ",";
    }
    std::cout << std::endl;
    std::cout << std::endl;
    for (int i = 0; i < internal->GetSize(); i++) {
      ToString(reinterpret_cast<BPlusTreePage *>(bpm->FetchPage(internal->ValueAt(i))->GetData()), bpm);
    }
  }
  bpm->UnpinPage(page->GetPageId(), false);
}

template class BPlusTree<GenericKey<4>, RID, GenericComparator<4>>;
template class BPlusTree<GenericKey<8>, RID, GenericComparator<8>>;
template class BPlusTree<GenericKey<16>, RID, GenericComparator<16>>;
template class BPlusTree<GenericKey<32>, RID, GenericComparator<32>>;
template class BPlusTree<GenericKey<64>, RID, GenericComparator<64>>;

}  // namespace bustub
