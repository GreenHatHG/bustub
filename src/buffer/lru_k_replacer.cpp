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
#include "common/exception.h"

namespace bustub {

LRUKReplacer::LRUKReplacer(size_t num_frames, size_t k) : replacer_size_(num_frames), k_(k) {}

auto LRUKReplacer::Evict(frame_id_t *frame_id) -> bool {
  auto attempt_eviction = [&](std::list<frame_id_t>& list) -> bool {
    for (auto it = list.begin(); it != list.end();) {
      auto entry = lru_entry_hash_[*it];
      if (entry.evictable_) {
        *frame_id = *it;
        lru_entry_hash_.erase(*it);
        list.erase(it);
        return true;
      }
      ++it;
    }
    return false;
  };

  if (attempt_eviction(more_then_k_list_)) {
    return true;
  }

  if (attempt_eviction(less_than_k_list_)) {
    return true;
  }

  return false;
}

void LRUKReplacer::RecordAccess(frame_id_t frame_id) {
  std::cout << "[LRUKReplacer::RecordAccess] " << "frame id: " << frame_id << std::endl;
  auto it = lru_entry_hash_.find(frame_id);
  auto already_existed = it != lru_entry_hash_.end();

  LruEntry entry;
  if(already_existed){
    entry = it->second;
  }

  entry.access_count_++;
  current_timestamp_++;
  hist_[std::make_pair(frame_id, entry.access_count_)] = current_timestamp_;

  lru_entry_hash_[frame_id] = entry;
}

void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
  auto is_frame_id_exists = lru_entry_hash_.find(frame_id) != lru_entry_hash_.end();
  if(!is_frame_id_exists){
    throw Exception("frame id is invalid");
  }
  if(!lru_entry_hash_[frame_id].evictable_ && set_evictable){
    curr_size_++;
    lru_entry_hash_[frame_id].evictable_ = true;
  }
  if(lru_entry_hash_[frame_id].evictable_ && !set_evictable){
    curr_size_--;
    lru_entry_hash_[frame_id].evictable_ = false;
  }
}

void LRUKReplacer::Remove(frame_id_t frame_id) {
  if(lru_entry_hash_.find(frame_id) == lru_entry_hash_.end()){
    return;
  }
  auto entry = lru_entry_hash_[frame_id];
  if(!entry.evictable_){
    return;
  }

  if(entry.access_count_ >= k_){
    more_then_k_list_.erase(entry.list_iter_);
  }else{
    less_than_k_list_.erase(entry.list_iter_);
  }
  lru_entry_hash_.erase(frame_id);
  curr_size_--;
  replacer_size_--;
}

auto LRUKReplacer::Size() -> size_t { return curr_size_; }

}  // namespace bustub
