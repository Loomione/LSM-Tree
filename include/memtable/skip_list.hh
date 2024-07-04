#pragma once

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <shared_mutex>
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

  int randomLevel();  // 随机生成层数

 public:
  SkipList(int maxLevel = MAX_LEVEL, float probability = P);  // 构造函数

  void                       insert(K key, V value);            // 插入
  bool                       search(K key, V &value) const;     // 查找
  bool                       remove(K key);                     // 删除
  std::list<std::pair<K, V>> rangeQuery(K start, K end) const;  // 范围查询
  void                       dump() const;                      // 输出整个跳表

  using iterator       = typename std::list<std::pair<K, V>>::iterator;
  using const_iterator = typename std::list<std::pair<K, V>>::const_iterator;

  bool empty() const;      // 判断是否为空
  V   &operator[](K key);  // 重载[]运算符
  V   &at(K key);          // 返回指定键的值

  iterator       begin();
  iterator       end();
  const_iterator begin() const;
  const_iterator end() const;
  iterator       lower_bound(K key);
};

// SkipList implementions

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
  std::unique_lock<std::shared_mutex>              lock(mutex);  // 写锁

  std::shared_ptr<SkipListNode<K, V>> x = head;
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
  std::shared_lock<std::shared_mutex> lock(mutex);  // 读锁
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
  std::unique_lock<std::shared_mutex>              lock(mutex);  // 写锁

  std::shared_ptr<SkipListNode<K, V>> x = head;
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
std::list<std::pair<K, V>> SkipList<K, V>::rangeQuery(K start, K end) const {
  std::shared_lock<std::shared_mutex> lock(mutex);  // 读锁
  std::list<std::pair<K, V>>          result;
  std::shared_ptr<SkipListNode<K, V>> x = head;

  for (int i = currentLevel; i >= 0; i--) {
    while (x->forward[i] && x->forward[i]->key < start) {
      x = x->forward[i];
    }
  }

  x = x->forward[0];
  while (x && x->key <= end) {
    if (x->key >= start) {
      result.push_back({x->key, x->value});
    }
    x = x->forward[0];
  }

  return result;
}

template <typename K, typename V>
bool SkipList<K, V>::empty() const {
  std::shared_lock<std::shared_mutex> lock(mutex);
  return head->forward[0] == nullptr;
}

template <typename K, typename V>
V &SkipList<K, V>::operator[](K key) {
  std::shared_lock<std::shared_mutex> lock(mutex);  // 读锁
  V                                   value;
  auto                                result = search(key, value);
  if (result) {
    return value;
  } else {
    insert(key, V());
    search(key, value);
    return value;
  }
}

template <typename K, typename V>
V &SkipList<K, V>::at(K key) {
  std::shared_lock<std::shared_mutex> lock(mutex);  // 读锁
  V                                   value;
  if (search(key, value)) {
    return value;
  } else {
    throw std::out_of_range("Key not found");
  }
}

template <typename K, typename V>
typename SkipList<K, V>::iterator SkipList<K, V>::begin() {
  std::shared_lock<std::shared_mutex> lock(mutex);  // 读锁
  return std::list<std::pair<K, V>>::iterator(head->forward[0]);
}

template <typename K, typename V>
typename SkipList<K, V>::iterator SkipList<K, V>::end() {
  std::shared_lock<std::shared_mutex> lock(mutex);  // 读锁
  return std::list<std::pair<K, V>>::iterator(nullptr);
}

template <typename K, typename V>
typename SkipList<K, V>::const_iterator SkipList<K, V>::begin() const {
  std::shared_lock<std::shared_mutex> lock(mutex);  // 读锁
  return std::list<std::pair<K, V>>::const_iterator(head->forward[0]);
}

template <typename K, typename V>
typename SkipList<K, V>::const_iterator SkipList<K, V>::end() const {
  std::shared_lock<std::shared_mutex> lock(mutex);  // 读锁
  return std::list<std::pair<K, V>>::const_iterator(nullptr);
}

template <typename K, typename V>
typename SkipList<K, V>::iterator SkipList<K, V>::lower_bound(K key) {
  std::shared_lock<std::shared_mutex> lock(mutex);  // 读锁
  std::shared_ptr<SkipListNode<K, V>> x = head;
  for (int i = currentLevel; i >= 0; i--) {
    while (x->forward[i] && x->forward[i]->key < key) {
      x = x->forward[i];
    }
  }
  x = x->forward[0];
  return std::list<std::pair<K, V>>::iterator(x);
}
}  // namespace lsm_tree
