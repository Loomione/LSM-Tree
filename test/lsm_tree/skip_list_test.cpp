#include "memtable/skip_list.hh"
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "gtest/gtest.h"

using namespace std;
using namespace lsm_tree;
class SkipListTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // 初始化 SkipList
    skiplist_ = make_unique<SkipList<int, string>>();
  }

  unique_ptr<SkipList<int, string>> skiplist_;
};

TEST_F(SkipListTest, InsertPerformance) {
  const int                 num_inserts = 1000000;
  vector<pair<int, string>> data;
  for (int i = 0; i < num_inserts; ++i) {
    data.emplace_back(i, "value" + to_string(i));
  }

  // Measure performance
  for (const auto &[key, value] : data) {
    skiplist_->insert(key, value);
  }
}

TEST_F(SkipListTest, SearchPerformance) {
  const int                 num_inserts = 1000000;
  vector<pair<int, string>> data;
  for (int i = 0; i < num_inserts; ++i) {
    data.emplace_back(i, "value" + to_string(i));
  }

  // Insert elements before testing search performance
  for (const auto &[key, value] : data) {
    skiplist_->insert(key, value);
  }

  // Measure performance
  string value;
  for (const auto &[key, _] : data) {
    skiplist_->search(key, value);
  }
}
