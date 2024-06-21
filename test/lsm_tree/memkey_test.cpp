#include "gtest/gtest.h"
#include "keys.hh"

using namespace lsm_tree;
using namespace std;

TEST(MemKey, NewMinMemKey) {
  std::string_view key     = "hello";
  auto             mem_key = MemKey::NewMinMemKey(key);
  EXPECT_EQ(mem_key.user_key_, key);
  EXPECT_EQ(mem_key.seq_, 0);
  EXPECT_EQ(mem_key.type_, OperatorType::PUT);
}

TEST(MemKey, ToSSTableKey) {
  MemKey mem_key("hello", 1, OperatorType::PUT);
  auto   sstable_key = mem_key.ToSSTableKey();
  auto   mem_key2    = MemKey();
  mem_key2.FromSSTableKey(sstable_key);
  EXPECT_EQ(mem_key.user_key_, mem_key2.user_key_);
  EXPECT_EQ(mem_key.seq_, mem_key2.seq_);
  EXPECT_EQ(mem_key.type_, mem_key2.type_);
}

TEST(MemKey, EncodeKV) {
  MemKey mem_key("hello", 1, OperatorType::PUT);
  string value    = "world";
  auto   kv       = EncodeKVPair(mem_key, value);
  auto   mem_key2 = MemKey();
  string value2;
  DecodeKVPair(kv, mem_key2, value2);
  EXPECT_EQ(mem_key.user_key_, mem_key2.user_key_);
  EXPECT_EQ(mem_key.seq_, mem_key2.seq_);
  EXPECT_EQ(mem_key.type_, mem_key2.type_);
  EXPECT_EQ(value, value2);
}

TEST(MemKey, Compare) {
  MemKey mem_key1("hello", 1, OperatorType::PUT);
  MemKey mem_key2("hello", 2, OperatorType::PUT);
  EXPECT_TRUE(mem_key1 > mem_key2);
  MemKey mem_key3("hello", 1, OperatorType::DELETE);
  EXPECT_TRUE(mem_key3 < mem_key1);
  auto inner_key1 = mem_key1.ToSSTableKey();
  auto inner_key2 = mem_key2.ToSSTableKey();
  auto inner_key3 = mem_key3.ToSSTableKey();
  auto ret        = CmpInnerKey(inner_key1, inner_key2);
  EXPECT_TRUE(ret > 0);
  ret = CmpInnerKey(inner_key3, inner_key1);
  EXPECT_TRUE(ret < 0);
}