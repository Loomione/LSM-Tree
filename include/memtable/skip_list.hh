//
// Created by 贾明卓 on 2024/7/3.
//

#ifndef LSM_TREE_SKIPLISTS_H
#define LSM_TREE_SKIPLISTS_H

#include <iostream>
#include <list>
#include <memory>
#include <shared_mutex>
#include <vector>

const int   MAX_LEVEL = 16;
const float P         = 0.5;

template <typename K, typename V>
class SkipListNode {
 public:
  K                                          key;
  V                                          value;
  std::vector<std::shared_ptr<SkipListNode>> forward;
  SkipListNode(K key, V value, int level);
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
  SkipList(int maxLevel = MAX_LEVEL, float probability = P);    // 构造函数
  void                       insert(K key, V value);            // 插入
  bool                       search(K key, V &value) const;     // 查找
  bool                       remove(K key);                     // 删除
  std::list<std::pair<K, V>> rangeQuery(K start, K end) const;  // 范围查询
  void                       dump() const;                      // 输出整个跳表
};
#endif  // LSM_TREE_SKIPLISTS_H
