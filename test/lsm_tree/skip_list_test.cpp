#include "memtable/skip_list.hh"
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "gtest/gtest.h"

using namespace std;

class SkipListTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // 初始化 SkipList
    skiplist_ = make_unique<SkipList<int, string>>();
  }

  unique_ptr<SkipList<int, string>> skiplist_;
};

TEST_F(SkipListTest, InsertAndSearch) {
  // 插入元素
  skiplist_->insert(1, "value1");
  skiplist_->insert(2, "value2");
  skiplist_->insert(3, "value3");

  // 搜索元素
  string value;
  EXPECT_TRUE(skiplist_->search(1, value));
  EXPECT_EQ(value, "value1");

  EXPECT_TRUE(skiplist_->search(2, value));
  EXPECT_EQ(value, "value2");

  EXPECT_TRUE(skiplist_->search(3, value));
  EXPECT_EQ(value, "value3");

  // 搜索不存在的元素
  EXPECT_FALSE(skiplist_->search(4, value));
}

TEST_F(SkipListTest, Remove) {
  // 插入元素
  skiplist_->insert(1, "value1");
  skiplist_->insert(2, "value2");
  skiplist_->insert(3, "value3");

  // 移除元素
  EXPECT_TRUE(skiplist_->remove(2));

  // 检查移除后的结果
  string value;
  EXPECT_FALSE(skiplist_->search(2, value));

  // 检查其他元素是否还存在
  EXPECT_TRUE(skiplist_->search(1, value));
  EXPECT_EQ(value, "value1");

  EXPECT_TRUE(skiplist_->search(3, value));
  EXPECT_EQ(value, "value3");

  // 尝试移除不存在的元素
  EXPECT_FALSE(skiplist_->remove(4));
}

TEST_F(SkipListTest, RangeQuery) {
  // 插入元素
  for (int i = 1; i <= 10; ++i) {
    skiplist_->insert(i, "value" + to_string(i));
  }

  // 进行范围查询
  auto result = skiplist_->rangeQuery(3, 7);

  // 检查范围查询结果
  vector<pair<int, string>> expected = {{3, "value3"}, {4, "value4"}, {5, "value5"}, {6, "value6"}, {7, "value7"}};

  EXPECT_EQ(result.size(), expected.size());
  auto it     = result.begin();
  auto exp_it = expected.begin();
  for (; it != result.end() && exp_it != expected.end(); ++it, ++exp_it) {
    EXPECT_EQ(it->first, exp_it->first);
    EXPECT_EQ(it->second, exp_it->second);
  }
}

TEST_F(SkipListTest, EmptyCheck) {
  // 跳表应该初始化为空
  EXPECT_TRUE(skiplist_->empty());

  // 插入元素后不为空
  skiplist_->insert(1, "value1");
  EXPECT_FALSE(skiplist_->empty());

  // 移除所有元素后再次为空
  skiplist_->remove(1);
  EXPECT_TRUE(skiplist_->empty());
}

TEST_F(SkipListTest, InsertPerformance) {
  const int                 num_inserts = 1000000;
  vector<pair<int, string>> data;
  for (int i = 0; i < num_inserts; ++i) {
    data.emplace_back(i, "value" + to_string(i));
  }

  // Measure performance
  auto start = chrono::high_resolution_clock::now();
  for (const auto &[key, value] : data) {
    skiplist_->insert(key, value);
  }
  auto                     end      = chrono::high_resolution_clock::now();
  chrono::duration<double> duration = end - start;
  cout << "SkipList insert duration: " << duration.count() << " seconds" << endl;
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
  auto   start = chrono::high_resolution_clock::now();
  string value;
  for (const auto &[key, _] : data) {
    skiplist_->search(key, value);
  }
  auto                     end      = chrono::high_resolution_clock::now();
  chrono::duration<double> duration = end - start;
  cout << "SkipList search duration: " << duration.count() << " seconds" << endl;
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
