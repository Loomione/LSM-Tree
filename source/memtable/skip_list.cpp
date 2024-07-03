/**
 * @file skip_list.cpp
 * @author jmz (jmz614639031@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-07-03
 *
 * @copyright Copyright (c) 2024
 *
 */

#include "memtable/skip_list.hh"
#include <cstdlib>
#include <ctime>

// 跳表实现

template <typename K, typename V>
SkipListNode<K, V>::SkipListNode(K key, V value, int level) : key(k), value(v), forward(level + 1) {}

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
  std::shared_lock<std::shared_mutex>              lock(mutex);  // 写所

  std::shared_ptr<SkipListNode<K, V>> x = head;

  // 从最高层开始查找
  for (int i = currentLevel; i >= 0; --i) {
    while (x->forward[i] && x->forward[i]->key < key) {
      x = x->forward[i];
    }
    update[i] = x;
  }

  int lvl = randomLevel();

  // 如果随机生成的层数大于当前层数，则更新update
  if (lvl > currentLevel) {
    for (int i = currentLevel + 1; i <= lvl; ++i) {
      update[i] = head;
    }
    currentLevel = lvl;
  }

  x = std::make_shared<SkipListNode<K, V>>(key, value, lvl);

  // 更新forward指针, 插入节点
  for (int i = 0; i <= lvl; ++i) {
    x->forward[i]         = update[i]->forward[i];
    update[i]->forward[i] = x;
  }
}
template <typename K, typename V>
bool SkipList<K, V>::search(K key, V &value) const {
  std::shared_lock<std::shared_mutex> lock(mutex);  // 读锁

  std::shared_ptr<SkipListNode<K, V>> x = head;

  // 从最高层开始查找
  for (int i = currentLevel; i >= 0; --i) {
    while (x->forward[i] && x->forward[i]->key < key) {
      x = x->forward[i];
    }
  }

  // x->forward[0]是第一层的第一个节点
  x = x->forward[0];

  if (x && x->key == key) {
    value = x->value;
    return true;
  }

  return false;
}

template <typename K, typename V>
bool SkipList<K, V>::remove(K key) {
  // 保存每一层的前驱节点
  std::vector<std::shared_ptr<SkipListNode<K, V>>> update(maxLevel + 1);
  std::unique_lock<std::shared_mutex>              lock(mutex);  // 写锁

  std::shared_ptr<SkipListNode<K, V>> x = head;
  for (int i = currentLevel; i >= 0; --i) {
    while (x->forward[i] && x->forward[i]->key < key) {
      x = x->forward[i];
    }
    update[i] = x;
  }

  x = x->forward[0];

  if (!x || x->key != key) {
    return false;
  }

  for (int i = 0; i <= currentLevel; ++i) {
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

  // 查找范围查询的起点
  for (int i = currentLevel; i >= 0; --i) {
    while (x->forward[i] && x->forward[i]->key < start) {
      x = x->forward[i];
    }
  }

  x = x->forward[0];

  // 收集范围内的所有值
  while (x && x->key <= end) {
    if (x->key >= start) {
      result.push_back(std::make_pair(x->key, x->value));
    }
    x = x->forward[0];
  }

  return result;
}

template <typename K, typename V>

void SkipList<K, V>::dump() const {
  std::shared_lock<std::shared_mutex> lock(mutex);  // 读锁
  for (int i = 0; i <= currentLevel; ++i) {
    std::shared_ptr<SkipListNode<K, V>> x = head->forward[i];
    std::cout << "Level " << i << ": ";
    while (x) {
      std::cout << x->key << " ";
      x = x->forward[i];
    }
    std::cout << std::endl;
  }
}