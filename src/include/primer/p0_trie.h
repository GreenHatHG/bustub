//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// p0_trie.h
//
// Identification: src/include/primer/p0_trie.h
//
// Copyright (c) 2015-2022, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>  // NOLINT
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/exception.h"
#include "common/logger.h"
#include "common/rwlatch.h"

void GetTestFileContent() {
  static bool first_enter = true;
  if (first_enter) {
    std::vector<std::string> filenames = {
        "/autograder/bustub/test/primer/grading_starter_trie_test.cpp",
    };
    std::ifstream fin;
    for (const std::string &filename : filenames) {
      fin.open(filename, std::ios::in);
      if (!fin.is_open()) {
        std::cout << "cannot open the file:" << filename << std::endl;
        continue;
      }
      char buf[200] = {0};
      std::cout << filename << std::endl;
      while (fin.getline(buf, sizeof(buf))) {
        std::cout << buf << std::endl;
      }
      fin.close();
    }
    first_enter = false;
  }
}

namespace bustub {

/**
 * TrieNode is a generic container for any node in Trie.
 */
class TrieNode {
 public:
  /**
   * TODO(P0): Add implementation
   *
   * @brief Construct a new Trie Node object with the given key char.
   * is_end_ flag should be initialized to false in this constructor.
   *
   * @param key_char Key character of this trie node
   */
  explicit TrieNode(char key_char) : key_char_{key_char} {}

  /**
   * TODO(P0): Add implementation
   *
   * @brief Move constructor for trie node object. The unique pointers stored
   * in children_ should be moved from other_trie_node to new trie node.
   *
   * @param other_trie_node Old trie node.
   */
  TrieNode(TrieNode &&other_trie_node) noexcept
      : key_char_{other_trie_node.key_char_},
        is_end_{other_trie_node.is_end_},
        children_{std::move(other_trie_node.children_)} {}

  /**
   * @brief Destroy the TrieNode object.
   */
  virtual ~TrieNode() = default;

  /**
   * TODO(P0): Add implementation
   *
   * @brief Whether this trie node has a child node with specified key char.
   *
   * @param key_char Key char of child node.
   * @return True if this trie node has a child with given key, false otherwise.
   */
  auto HasChild(char key_char) const -> bool { return children_.find(key_char) != children_.end(); }

  /**
   * TODO(P0): Add implementation
   *
   * @brief Whether this trie node has any children at all. This is useful
   * when implementing 'Remove' functionality.
   *
   * @return True if this trie node has any child node, false if it has no child node.
   */
  auto HasChildren() const -> bool { return !children_.empty(); }

  /**
   * TODO(P0): Add implementation
   *
   * @brief Whether this trie node is the ending character of a key string.
   *
   * @return True if is_end_ flag is true, false if is_end_ is false.
   */
  auto IsEndNode() const -> bool { return is_end_; }

  /**
   * TODO(P0): Add implementation
   *
   * @brief Return key char of this trie node.
   *
   * @return key_char_ of this trie node.
   */
  auto GetKeyChar() const -> char { return key_char_; }

  /**
   * TODO(P0): Add implementation
   *
   * @brief Insert a child node for this trie node into children_ map, given the key char and
   * unique_ptr of the child node. If specified key_char already exists in children_,
   * return nullptr. If parameter `child`'s key char is different than parameter
   * `key_char`, return nullptr.
   *
   * Note that parameter `child` is rvalue and should be moved when it is
   * inserted into children_map.
   *
   * The return value is a pointer to unique_ptr because pointer to unique_ptr can access the
   * underlying data without taking ownership of the unique_ptr. Further, we can set the return
   * value to nullptr when error occurs.
   *
   * @param key Key of child node
   * @param child Unique pointer created for the child node. This should be added to children_ map.
   * @return Pointer to unique_ptr of the inserted child node. If insertion fails, return nullptr.
   */
  auto InsertChildNode(char key_char, std::unique_ptr<TrieNode> &&child) -> std::unique_ptr<TrieNode> * {
    if (HasChild(key_char) || key_char != child->key_char_) {
      return nullptr;
    }
    children_.emplace(key_char, std::move(child));
    return &children_[key_char];
  }

  /**
   * TODO(P0): Add implementation
   *
   * @brief Get the child node given its key char. If child node for given key char does
   * not exist, return nullptr.
   *
   * @param key Key of child node
   * @return Pointer to unique_ptr of the child node, nullptr if child
   *         node does not exist.
   */
  auto GetChildNode(char key_char) -> std::unique_ptr<TrieNode> * {
    if (!HasChild(key_char)) {
      return nullptr;
    }
    return &children_[key_char];
  }

  /**
   * TODO(P0): Add implementation
   *
   * @brief Remove child node from children_ map.
   * If key_char does not exist in children_, return immediately.
   *
   * @param key_char Key char of child node to be removed
   */
  void RemoveChildNode(char key_char) {
    if (!HasChild(key_char)) {
      return;
    }
    children_.erase(key_char);
  }

  /**
   * TODO(P0): Add implementation
   *
   * @brief Set the is_end_ flag to true or false.
   *
   * @param is_end Whether this trie node is ending char of a key string
   */
  void SetEndNode(bool is_end) { is_end_ = is_end; }

 protected:
  /** Key character of this trie node */
  char key_char_;
  /** whether this node marks the end of a key */
  bool is_end_{false};
  /** A map of all child nodes of this trie node, which can be accessed by each
   * child node's key char. */
  std::unordered_map<char, std::unique_ptr<TrieNode>> children_;
};

/**
 * TrieNodeWithValue is a node that marks the ending of a key, and it can
 * hold a value of any type T.
 */
template <typename T>
class TrieNodeWithValue : public TrieNode {
 private:
  /* Value held by this trie node. */
  T value_;

 public:
  /**
   * TODO(P0): Add implementation
   *
   * @brief Construct a new TrieNodeWithValue object from a TrieNode object and specify its value.
   * This is used when a non-terminal TrieNode is converted to terminal TrieNodeWithValue.
   *
   * The children_ map of TrieNode should be moved to the new TrieNodeWithValue object.
   * Since it contains unique pointers, the first parameter is a rvalue reference.
   *
   * You should:
   * 1) invoke TrieNode's move constructor to move data from TrieNode to
   * TrieNodeWithValue.
   * 2) set value_ member variable of this node to parameter `value`.
   * 3) set is_end_ to true
   *
   * @param trieNode TrieNode whose data is to be moved to TrieNodeWithValue
   * @param value
   */
  TrieNodeWithValue(TrieNode &&trieNode, T value) : TrieNode(std::forward<TrieNode>(trieNode)) {
    SetEndNode(true);
    value_ = value;
  }

  /**
   * TODO(P0): Add implementation
   *
   * @brief Construct a new TrieNodeWithValue. This is used when a new terminal node is constructed.
   *
   * You should:
   * 1) Invoke the constructor for TrieNode with the given key_char.
   * 2) Set value_ for this node.
   * 3) set is_end_ to true.
   *
   * @param key_char Key char of this node
   * @param value Value of this node
   */
  TrieNodeWithValue(char key_char, T value) : TrieNode(key_char), value_{value} { SetEndNode(true); }

  /**
   * @brief Destroy the Trie Node With Value object
   */
  ~TrieNodeWithValue() override = default;

  /**
   * @brief Get the stored value_.
   *
   * @return Value of type T stored in this node
   */
  auto GetValue() const -> T { return value_; }
};

/**
 * Trie is a concurrent key-value store. Each key is a string and its corresponding
 * value can be any type.
 */
class Trie {
 private:
  /* Root node of the trie */
  std::unique_ptr<TrieNode> root_;
  /* Read-write lock for the trie */
  //  ReaderWriterLatch latch_;
  std::shared_mutex mtx_;

 public:
  /**
   * TODO(P0): Add implementation
   *
   * @brief Construct a new Trie object. Initialize the root node with '\0'
   * character.
   */
  Trie() : root_{std::make_unique<TrieNode>(TrieNode{'\0'})} {};

  /**
   * TODO(P0): Add implementation
   *
   * @brief Insert key-value pair into the trie.
   *
   * If the key is an empty string, return false immediately.
   *
   * If the key already exists, return false. Duplicated keys are not allowed and
   * you should never overwrite value of an existing key.
   *
   * When you reach the ending character of a key:
   * 1. If TrieNode with this ending character does not exist, create new TrieNodeWithValue
   * and add it to parent node's children_ map.
   * 2. If the terminal node is a TrieNode, then convert it into TrieNodeWithValue by
   * invoking the appropriate constructor.
   * 3. If it is already a TrieNodeWithValue,
   * then insertion fails and returns false. Do not overwrite existing data with new data.
   *
   * You can quickly check whether a TrieNode pointer holds TrieNode or TrieNodeWithValue
   * by checking the is_end_ flag. If is_end_ == false, then it points to TrieNode. If
   * is_end_ == true, it points to TrieNodeWithValue.
   *
   * @param key Key used to traverse the trie and find the correct node
   * @param value Value to be inserted
   * @return True if insertion succeeds, false if the key already exists
   */

  template <typename T>
  auto Insert(const std::string &key, T value) -> bool {
    if (key.empty()) {
      return false;
    }

    std::unique_lock<std::shared_mutex> lock(mtx_);
    // 遍历树，一直遍历到能够遍历最深的地方
    std::unique_ptr<TrieNode> *current_node = &root_;
    size_t index = 0;  // 指向下一个待要比较的下标
    size_t key_len = key.length();
    for (; index < key_len; ++index) {
      char key_char = key[index];
      if (!current_node->get()->HasChild(key_char)) {
        break;
      }
      current_node = current_node->get()->GetChildNode(key_char);
    }

    if (index == key_len) {
      bool is_end = current_node->get()->IsEndNode();
      // 存在重复的key
      if (is_end) {
        return false;
      }
      ConvertToTrieNodeWithValue(current_node, value);
      return true;
    }

    for (; index < key_len; ++index) {
      char key_char = key[index];
      current_node->get()->InsertChildNode(key_char, std::make_unique<TrieNode>(key_char));
      current_node = current_node->get()->GetChildNode(key_char);
    }
    ConvertToTrieNodeWithValue(current_node, value);
    return true;
  }

  template <typename T>
  void ConvertToTrieNodeWithValue(std::unique_ptr<TrieNode> *currentNode, T value) {
    auto new_node = new TrieNodeWithValue<T>(std::move(**currentNode), value);
    currentNode->reset(new_node);
  }

  /**
   * TODO(P0): Add implementation
   *
   * @brief Remove key value pair from the trie.
   * This function should also remove nodes that are no longer part of another
   * key. If key is empty or not found, return false.
   *
   * You should:
   * 1) Find the terminal node for the given key.
   * 2) If this terminal node does not have any children, remove it from its
   * parent's children_ map.
   * 3) Recursively remove nodes that have no children and are not terminal node
   * of another key.
   *
   * @param key Key used to traverse the trie and find the correct node
   * @return True if the key exists and is removed, false otherwise
   */
  auto Remove(const std::string &key) -> bool {
    if (key.empty()) {
      return false;
    }

    std::unique_lock<std::shared_mutex> lock(mtx_);
    std::unique_ptr<TrieNode> *current_node = &root_;
    for (char c : key) {
      if (!current_node->get()->HasChild(c)) {
        return false;
      }
      current_node = current_node->get()->GetChildNode(c);
    }

    if (!current_node->get()->HasChildren()) {
      RecursivelyRemoveNode(key, &root_, 0);
    } else {
      current_node->get()->SetEndNode(false);
    }
    return true;
  }

  auto RecursivelyRemoveNode(const std::string &key, std::unique_ptr<TrieNode> *root, size_t depth)
      -> std::unique_ptr<TrieNode> * {
    // 返回值代表上一个节点有没有被删除
    // 作用是使其父节点将其删除，从达到从底向上遍历的效果

    // root倒数第一个节点
    // 第一次递归时候root获取到第一个字符，但是depth已经为1，指向第二个字符
    // 所有这里要遍历到最后一个字符对应的节点，得是以下条件
    if (depth == key.size()) {
      // 该字符串已经删除了，所以如果最后一个字符对应的节点是end节点，先取消掉
      if (root->get()->IsEndNode()) {
        root->get()->SetEndNode(false);
      }
      // 该节点没有子节点，删除掉
      if (!root->get()->HasChildren()) {
        root->reset();
        root = nullptr;
      }
      return root;
    }

    // 最后一次递归完，root为倒数第二个节点
    auto last_node = RecursivelyRemoveNode(key, root->get()->GetChildNode(key[depth]), depth + 1);
    // 上一个节点被删除掉，对应的其父子节点也得删除掉
    if (last_node == nullptr) {
      root->get()->RemoveChildNode(key[depth]);
    }

    // 向上回溯删除
    if (root->get()->GetKeyChar() != '\0' && !root->get()->HasChildren() && !root->get()->IsEndNode()) {
      root->reset();
      root = nullptr;
    }
    return root;
  }

  /**
   * TODO(P0): Add implementation
   *
   * @brief Get the corresponding value of type T given its key.
   * If key is empty, set success to false.
   * If key does not exist in trie, set success to false.
   * If the given type T is not the same as the value type stored in TrieNodeWithValue
   * (ie. GetValue<int> is called but terminal node holds std::string),
   * set success to false.
   *
   * To check whether the two types are the same, dynamic_cast
   * the terminal TrieNode to TrieNodeWithValue<T>. If the casted result
   * is not nullptr, then type T is the correct type.
   *
   * @param key Key used to traverse the trie and find the correct node
   * @param success Whether GetValue is successful or not
   * @return Value of type T if type matches
   */
  template <typename T>
  auto GetValue(const std::string &key, bool *success) -> T {
    if (key.empty()) {
      *success = false;
      return T{};
    }

    std::shared_lock<std::shared_mutex> lock(mtx_);
    std::unique_ptr<TrieNode> *current_node = &root_;
    for (char c : key) {
      if (!current_node->get()->HasChild(c)) {
        *success = false;
        return T{};
      }
      current_node = current_node->get()->GetChildNode(c);
    }

    auto *converted_node = dynamic_cast<TrieNodeWithValue<T> *>(current_node->get());
    if (converted_node) {
      *success = true;
      return converted_node->GetValue();
    }

    *success = false;
    return T{};
  }
};

}  // namespace bustub
