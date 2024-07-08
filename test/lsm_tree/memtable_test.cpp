#include "memtable/memtable.hh"
#include <gtest/gtest.h>
#include <chrono>
#include <iostream>
#include <string>
#include "memtable/keys.hh"
#include "options.hh"

using namespace lsm_tree;
using namespace std;

class MemTableTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // 初始化 MemTable
    db_options_ = make_unique<DBOptions>();
    memtable_   = make_unique<MemTable>(*db_options_);
  }

  unique_ptr<DBOptions> db_options_;
  unique_ptr<MemTable>  memtable_;
};

TEST_F(MemTableTest, EmptyCheck) {
  // 测试 MemTable 初始化为空
  EXPECT_TRUE(memtable_->Empty());

  // 插入元素后不为空
  MemKey key("test_key", 1, OperatorType::PUT);
  memtable_->Put(key, "test_value");
  EXPECT_FALSE(memtable_->Empty());
}

TEST_F(MemTableTest, PutAndGet) {
  // 插入并查询元素
  MemKey key("test_key", 1, OperatorType::PUT);
  string value("test_value");
  memtable_->Put(key, value);

  string result;
  EXPECT_EQ(memtable_->Get("test_key", result), RC::OK);
  EXPECT_EQ(result, value);

  // 测试查询不存在的元素
  EXPECT_EQ(memtable_->Get("nonexistent_key", result), RC::NOT_FOUND);
}

TEST_F(MemTableTest, UpdateValue) {
  // 插入并更新元素
  MemKey key("test_key", 1, OperatorType::PUT);
  string value1("value1");
  memtable_->Put(key, value1);

  string result;
  EXPECT_EQ(memtable_->Get("test_key", result), RC::OK);
  EXPECT_EQ(result, value1);

  // 更新值
  string value2("value2");
  memtable_->Put(key, value2);
  EXPECT_EQ(memtable_->Get("test_key", result), RC::OK);
  EXPECT_EQ(result, value2);
}

TEST_F(MemTableTest, PutPerformance) {
  const int    num_inserts = 1000000;
  const string value       = "value";

  // 测量插入性能
  auto start = chrono::high_resolution_clock::now();
  for (int i = 0; i < num_inserts; ++i) {
    string key = "key" + to_string(i);
    MemKey mem_key(key, i, OperatorType::PUT);
    memtable_->Put(mem_key, value);
  }
  auto                     end      = chrono::high_resolution_clock::now();
  chrono::duration<double> duration = end - start;
  cout << "MemTable insert duration: " << duration.count() << " seconds" << endl;
}

TEST_F(MemTableTest, GetPerformance) {
  const int    num_inserts = 1000000;
  const string value       = "value";

  // 在测试 Get 性能之前插入元素
  for (int i = 0; i < num_inserts; ++i) {
    string key = "key" + to_string(i);
    MemKey mem_key(key, i, OperatorType::PUT);
    memtable_->Put(mem_key, value);
  }

  // 测量查找性能
  auto start = chrono::high_resolution_clock::now();
  for (int i = 0; i < num_inserts; ++i) {
    string key = "key" + to_string(i);
    string result;
    memtable_->Get(key, result);
  }
  auto                     end      = chrono::high_resolution_clock::now();
  chrono::duration<double> duration = end - start;
  cout << "MemTable get duration: " << duration.count() << " seconds" << endl;
}

TEST_F(MemTableTest, Remove) {
  // 测试插入和删除功能
  MemKey key("test_key", 1, OperatorType::PUT);
  string value("test_value");
  memtable_->Put(key, value);

  // 查询确认插入成功
  string result;
  EXPECT_EQ(memtable_->Get("test_key", result), RC::OK);
  EXPECT_EQ(result, value);

  // 删除元素并确认删除成功
  key = MemKey("test_key", 1, OperatorType::DELETE);
  memtable_->Put(key, value);
  EXPECT_EQ(memtable_->Get("test_key", result), RC::NOT_FOUND);
}
