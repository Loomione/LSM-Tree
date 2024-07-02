#pragma once

#include <map>
#include <memory>
#include <shared_mutex>
#include <string>
#include "keys.hh"
#include "wal.hh"

namespace lsm_tree {

struct DBOptions;
class MemTable {
 public:
  struct Stat {
    Stat() = default;
    void Update(size_t key_size, size_t value_size) {
      keys_size_ += key_size;
      values_size_ += value_size;
    }
    auto   Sum() -> size_t { return keys_size_ + values_size_; }
    size_t keys_size_{0};
    size_t values_size_{0};
  };
  explicit MemTable(const DBOptions &options);
  MemTable(const DBOptions &options, WAL *wal);
  MemTable(const MemTable &)                   = delete;
  auto operator=(const MemTable &) -> MemTable = delete;

  ~MemTable() = default;

 public:
  auto Empty() -> bool;                                                           // 判断MemTable是否为空
  auto Put(const MemKey &key, string_view value) -> RC;                           // 插入数据
  auto PutTeeWAL(const MemKey &key, string_view value) -> RC;                     // 插入数据, 并写入WAL
  auto GetMemTableSize() -> size_t;                                               // 获取MemTable的大小
  auto DropWAL() -> RC;                                                           // 删除WAL
  auto Get(string_view key, string &value, int64_t seq = INT64_MAX) -> RC;        // 查询数据
  auto GetNoLock(string_view key, string &value, int64_t seq = INT64_MAX) -> RC;  // 查询数据, 不加锁
  auto BuildSSTable(string_view dbname, FileMetaData **meta_data_pointer) -> RC;  // 构建SSTable
  auto ForEachNoLock(std::function<RC(const MemKey &key, string_view value)> &&func) -> RC;  // 遍历数据, 不加锁

 private:
  Stat                          stat_;
  std::unique_ptr<WAL>          wal_;
  mutable std::shared_mutex     mtx_;
  const DBOptions              *options_;
  std::map<MemKey, std::string> table_;
};

}  // namespace lsm_tree