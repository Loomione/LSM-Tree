#pragma once
#include <cstdlib>
#include <ctime>
#include <iterator>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace lsm_tree {

const int   MAX_LEVEL = 16;
const float P         = 0.5;

template <typename K, typename V>
class SkipListNode {
 public:
  K                                          key;
  V                                          value;
  std::vector<std::shared_ptr<SkipListNode>> forward;

  SkipListNode(K key, V value, int level) : key(key), value(value), forward(level + 1) {}
};

template <typename K, typename V>
class SkipList {
 private:
  std::shared_ptr<SkipListNode<K, V>> head;
  int                                 maxLevel;
  int                                 currentLevel;
  float                               probability;
  mutable std::shared_mutex           mutex;

  int randomLevel();

 public:
  SkipList(int maxLevel = MAX_LEVEL, float probability = P);

  void insert(K key, V value);
  bool search(K key, V &value) const;
  bool remove(K key);
  bool empty() const;

  V       &operator[](const K &key);
  const V &operator[](const K &key) const;

  class Iterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = std::pair<const K, V>;
    using pointer           = value_type *;
    using reference         = value_type &;

    Iterator() : node_(nullptr) {}
    Iterator(std::shared_ptr<SkipListNode<K, V>> node) : node_(node) {}

    value_type operator*() const {
      value_ = std::make_pair(node_->key, node_->value);
      return value_;
    }
    pointer operator->() const { return &value_; }

    Iterator &operator++() {
      node_ = node_->forward[0];
      return *this;
    }

    Iterator operator++(int) {
      Iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    friend bool operator==(const Iterator &a, const Iterator &b) { return a.node_ == b.node_; }
    friend bool operator!=(const Iterator &a, const Iterator &b) { return a.node_ != b.node_; }

   private:
    std::shared_ptr<SkipListNode<K, V>> node_;
    mutable value_type                  value_;
  };

  class ConstIterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = std::pair<const K, V>;
    using pointer           = const value_type *;
    using reference         = const value_type &;

    ConstIterator() : node_(nullptr) {}
    ConstIterator(std::shared_ptr<SkipListNode<K, V>> node) : node_(node) {}

    value_type operator*() const {
      value_ = std::make_pair(node_->key, node_->value);
      return value_;
    }
    pointer operator->() const { return &value_; }

    ConstIterator &operator++() {
      node_ = node_->forward[0];
      return *this;
    }

    ConstIterator operator++(int) {
      ConstIterator tmp = *this;
      ++(*this);
      return tmp;
    }

    friend bool operator==(const ConstIterator &a, const ConstIterator &b) { return a.node_ == b.node_; }
    friend bool operator!=(const ConstIterator &a, const ConstIterator &b) { return a.node_ != b.node_; }

   private:
    std::shared_ptr<SkipListNode<K, V>> node_;
    mutable value_type                  value_;
  };

  Iterator      begin();
  Iterator      end();
  ConstIterator cbegin() const;
  ConstIterator cend() const;
  Iterator      lower_bound(const K &key);
};

// SkipList 类实现
template <typename K, typename V>
SkipList<K, V>::SkipList(int maxLevel, float probability)
    : maxLevel(maxLevel), probability(probability), currentLevel(0) {
  head = std::make_shared<SkipListNode<K, V>>(K(), V(), maxLevel);
  srand(static_cast<unsigned>(time(nullptr)));
}

template <typename K, typename V>
int SkipList<K, V>::randomLevel() {
  int level = 0;
  while (rand() / static_cast<double>(RAND_MAX) < probability && level < maxLevel) {
    level++;
  }
  return level;
}

template <typename K, typename V>
void SkipList<K, V>::insert(K key, V value) {
  std::vector<std::shared_ptr<SkipListNode<K, V>>> update(maxLevel + 1);
  std::unique_lock<std::shared_mutex>              lock(mutex);
  std::shared_ptr<SkipListNode<K, V>>              x = head;
  for (int i = currentLevel; i >= 0; i--) {
    while (x->forward[i] && x->forward[i]->key < key) {
      x = x->forward[i];
    }
    update[i] = x;
  }
  int lvl = randomLevel();
  if (lvl > currentLevel) {
    for (int i = currentLevel + 1; i <= lvl; i++) {
      update[i] = head;
    }
    currentLevel = lvl;
  }
  x = std::make_shared<SkipListNode<K, V>>(key, value, lvl);
  for (int i = 0; i <= lvl; i++) {
    x->forward[i]         = update[i]->forward[i];
    update[i]->forward[i] = x;
  }
}

template <typename K, typename V>
bool SkipList<K, V>::search(K key, V &value) const {
  std::shared_lock<std::shared_mutex> lock(mutex);
  std::shared_ptr<SkipListNode<K, V>> x = head;
  for (int i = currentLevel; i >= 0; i--) {
    while (x->forward[i] && x->forward[i]->key < key) {
      x = x->forward[i];
    }
  }
  x = x->forward[0];
  if (x && x->key == key) {
    value = x->value;
    return true;
  }
  return false;
}

template <typename K, typename V>
bool SkipList<K, V>::remove(K key) {
  std::vector<std::shared_ptr<SkipListNode<K, V>>> update(maxLevel + 1);
  std::unique_lock<std::shared_mutex>              lock(mutex);
  std::shared_ptr<SkipListNode<K, V>>              x = head;
  for (int i = currentLevel; i >= 0; i--) {
    while (x->forward[i] && x->forward[i]->key < key) {
      x = x->forward[i];
    }
    update[i] = x;
  }
  x = x->forward[0];
  if (!x || x->key != key) {
    return false;
  }
  for (int i = 0; i <= currentLevel; i++) {
    if (update[i]->forward[i] != x) {
      break;
    }
    update[i]->forward[i] = x->forward[i];
  }
  while (currentLevel > 0 && !head->forward[currentLevel]) {
    currentLevel--;
  }
  return true;
}

template <typename K, typename V>
bool SkipList<K, V>::empty() const {
  std::shared_lock<std::shared_mutex> lock(mutex);
  return head->forward[0] == nullptr;
}

template <typename K, typename V>
typename SkipList<K, V>::Iterator SkipList<K, V>::begin() {
  std::shared_lock<std::shared_mutex> lock(mutex);
  return Iterator(head->forward[0]);
}

template <typename K, typename V>
typename SkipList<K, V>::Iterator SkipList<K, V>::end() {
  return Iterator(nullptr);
}

template <typename K, typename V>
typename SkipList<K, V>::ConstIterator SkipList<K, V>::cbegin() const {
  std::shared_lock<std::shared_mutex> lock(mutex);
  return ConstIterator(head->forward[0]);
}

template <typename K, typename V>
typename SkipList<K, V>::ConstIterator SkipList<K, V>::cend() const {
  return ConstIterator(nullptr);
}

template <typename K, typename V>
typename SkipList<K, V>::Iterator SkipList<K, V>::lower_bound(const K &key) {
  std::shared_lock<std::shared_mutex> lock(mutex);
  std::shared_ptr<SkipListNode<K, V>> x = head;
  for (int i = currentLevel; i >= 0; i--) {
    while (x->forward[i] && x->forward[i]->key < key) {
      x = x->forward[i];
    }
  }
  x = x->forward[0];
  return Iterator(x);
}

// 实现 operator[]
template <typename K, typename V>
V &SkipList<K, V>::operator[](const K &key) {
  std::unique_lock<std::shared_mutex> lock(mutex);
  std::shared_ptr<SkipListNode<K, V>> x = head;
  for (int i = currentLevel; i >= 0; i--) {
    while (x->forward[i] && x->forward[i]->key < key) {
      x = x->forward[i];
    }
  }
  x = x->forward[0];
  if (x && x->key == key) {
    return x->value;
  } else {
    V default_value{};
    insert(key, default_value);    // Insert with default value if key not found
    return this->operator[](key);  // Retrying to get the value after insertion
  }
}

template <typename K, typename V>
const V &SkipList<K, V>::operator[](const K &key) const {
  std::shared_lock<std::shared_mutex> lock(mutex);
  std::shared_ptr<SkipListNode<K, V>> x = head;
  for (int i = currentLevel; i >= 0; i--) {
    while (x->forward[i] && x->forward[i]->key < key) {
      x = x->forward[i];
    }
  }
  x = x->forward[0];
  if (x && x->key == key) {
    return x->value;
  } else {
    throw std::out_of_range("Key not found in SkipList");
  }
}

}  // namespace lsm_tree
