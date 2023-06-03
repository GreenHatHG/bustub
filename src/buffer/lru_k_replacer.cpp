//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_k_replacer.cpp
//
// Identification: src/buffer/lru_k_replacer.cpp
//
// Copyright (c) 2015-2022, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/lru_k_replacer.h"
#include <algorithm>
#include "common/exception.h"

namespace bustub {

LRUKReplacer::LRUKReplacer(size_t num_frames, size_t k) : replacer_size_(num_frames), k_(k) {}

auto LRUKReplacer::Evict(frame_id_t *frame_id) -> bool {
  std::scoped_lock<std::mutex> lock(latch_);

  for (auto it = less_than_k_frames_.begin(); it != less_than_k_frames_.end(); ++it) {
    auto entry = lru_entry_hash_[*it];
    if (entry.evictable_) {
      *frame_id = *it;
      std::cout << "LRUKReplacer::Evict frame id: " << *frame_id << std::endl;

      lru_entry_hash_.erase(*it);
      frame_iterator_map_.erase(*it);
      less_than_k_frames_.erase(it);

      curr_size_--;
      PrintDebug();
      return true;
    }
  }

  std::sort(more_than_k_frames_.begin(), more_than_k_frames_.end(),
            [&](frame_id_t a, frame_id_t b) { return GetLastKAccessTime(a) < GetLastKAccessTime(b); });

  for (auto it = more_than_k_frames_.begin(); it != more_than_k_frames_.end(); ++it) {
    auto entry = lru_entry_hash_[*it];
    if (entry.evictable_) {
      *frame_id = *it;
      std::cout << "LRUKReplacer::Evict frame id: " << *frame_id << std::endl;

      lru_entry_hash_.erase(*it);
      more_than_k_frames_.erase(it);

      curr_size_--;
      PrintDebug();
      return true;
    }
  }

  return false;
}

void LRUKReplacer::RecordAccess(frame_id_t frame_id) {
  std::scoped_lock<std::mutex> lock(latch_);

  std::cout << "[LRUKReplacer::RecordAccess] "
            << "frame id: " << frame_id << std::endl;
  current_timestamp_++;

  LruEntry entry;
  if (lru_entry_hash_.find(frame_id) != lru_entry_hash_.end()) {
    entry = lru_entry_hash_[frame_id];
  } else {
    entry = LruEntry{};
    less_than_k_frames_.push_back(frame_id);
    frame_iterator_map_[frame_id] = std::prev(less_than_k_frames_.end());
  }

  entry.access_count_++;
  hist_[std::make_pair(frame_id, entry.access_count_)] = current_timestamp_;

  if (entry.access_count_ == k_) {
    less_than_k_frames_.erase(frame_iterator_map_[frame_id]);
    frame_iterator_map_.erase(frame_id);
    more_than_k_frames_.push_back(frame_id);
  }

  lru_entry_hash_[frame_id] = entry;
  PrintDebug();
}

void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
  std::scoped_lock<std::mutex> lock(latch_);

  if (lru_entry_hash_.find(frame_id) == lru_entry_hash_.end()) {
    return;
  }
  if (!lru_entry_hash_[frame_id].evictable_ && set_evictable) {
    curr_size_++;
    lru_entry_hash_[frame_id].evictable_ = true;
  }
  if (lru_entry_hash_[frame_id].evictable_ && !set_evictable) {
    curr_size_--;
    lru_entry_hash_[frame_id].evictable_ = false;
  }
}

void LRUKReplacer::Remove(frame_id_t frame_id) {
  std::scoped_lock<std::mutex> lock(latch_);

  if (lru_entry_hash_.find(frame_id) == lru_entry_hash_.end() || !lru_entry_hash_[frame_id].evictable_) {
    return;
  }

  if (frame_iterator_map_.find(frame_id) != frame_iterator_map_.end()) {
    less_than_k_frames_.erase(frame_iterator_map_[frame_id]);
  } else {
    auto remove_it = std::remove(more_than_k_frames_.begin(), more_than_k_frames_.end(), frame_id);
    more_than_k_frames_.erase(remove_it, more_than_k_frames_.end());
  }

  lru_entry_hash_.erase(frame_id);
  curr_size_--;
  replacer_size_--;
}

auto LRUKReplacer::Size() -> size_t { return curr_size_; }

}  // namespace bustub
