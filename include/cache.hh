/**
 * @file cache.hh
 * @author gusj (guchee@163.com)
 * @brief
 * @version 0.1
 * @date 2024-05-13
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#include <algorithm>
#include <list>
#include <mutex>
#include <unordered_map>
#include <utility>
namespace lsm_tree {

class NullLock {
 public:
  void lock() {}
  void unlock() {}
  bool try_lock() { return true; }
};

template <typename Key, typename Value, class Lock = NullLock>
class LRUCache {
 public:
  LRUCache(const LRUCache &)                     = delete;
  auto operator=(const LRUCache &) -> LRUCache & = delete;

  explicit LRUCache(size_t maxSize = 64) : max_size_(maxSize) {}
  virtual ~LRUCache() = default;

  void Put(const Key &k, const Value &v);
  auto Get(const Key &k, Value &v) -> bool;
  auto Remove(const Key &k) -> bool;
  auto Empty() const -> bool;
  auto Size() const -> size_t;
  void Clear();

 protected:
  void Prune();

 private:
  using Cache = std::list<std::pair<Key, Value>>;
  using Hash  = std::unordered_map<Key, typename Cache::iterator>;

  size_t max_size_;
  Cache  cache_;
  Hash   hash_;

  mutable Lock lock_;
};

template <typename Key, typename Value, class Lock>
auto LRUCache<Key, Value, Lock>::Size() const -> size_t {
  std::lock_guard<Lock> g(lock_);
  return cache_.size();
}

template <typename Key, typename Value, class Lock>
auto LRUCache<Key, Value, Lock>::Empty() const -> bool {
  std::lock_guard<Lock> g(lock_);
  return cache_.empty();
}

template <typename Key, typename Value, class Lock>
void LRUCache<Key, Value, Lock>::Clear() {
  std::lock_guard<Lock> g(lock_);
  hash_.clear();
  cache_.clear();
}

template <typename Key, typename Value, class Lock>
void LRUCache<Key, Value, Lock>::Put(const Key &k, const Value &v) {
  std::lock_guard<Lock> g(lock_);
  if (const auto iter = hash_.find(k); iter != hash_.end()) {
    iter->second->second = v;
    cache_.splice(cache_.begin(), cache_, iter->second);
    return;
  }
  cache_.emplace_front(k, v);
  hash_[k] = cache_.begin();
  if (hash_.size() >= max_size_) {
    Prune();
  }
}

template <typename Key, typename Value, class Lock>
auto LRUCache<Key, Value, Lock>::Get(const Key &k, Value &v) -> bool {
  std::lock_guard<Lock> g(lock_);

  const auto iter = hash_.find(k);
  if (iter == hash_.end()) {
    return false;
  }
  cache_.splice(cache_.begin(), cache_, iter->second);
  v = iter->second->second;
  return true;
}

template <typename Key, typename Value, class Lock>
auto LRUCache<Key, Value, Lock>::Remove(const Key &k) -> bool {
  std::lock_guard<Lock> g(lock_);

  const auto iter = hash_.find(k);
  if (iter == hash_.end()) {
    return false;
  }
  cache_.erase(iter->second);
  hash_.erase(iter);
  return true;
}
template <typename Key, typename Value, class Lock>
void LRUCache<Key, Value, Lock>::Prune() {
  while (hash_.size() > max_size_) {
    hash_.erase(cache_.back().first);
    cache_.pop_back();
  }
}

}  // namespace lsm_tree
