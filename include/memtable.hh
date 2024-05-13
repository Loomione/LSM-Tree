#pragma once
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include "wal.hh"
namespace lsm_tree {

struct DBOptions;
class MemTable {
 public:
  struct Stat {};
  explicit MemTable(const DBOptions &options);
  MemTable(const DBOptions &options, WAL *wal);
  MemTable(const MemTable &)                   = delete;
  auto operator=(const MemTable &) -> MemTable = delete;

  ~MemTable() = default;

 public:
  auto Put(const MemKey &key, string_view value) -> RC;
  auto PutTeeWAL(const MemKey &key, string_view value) -> RC;
  auto Get(string_view key, string &value, int64_t seq = INT64_MAX) -> RC;
  auto GetNoLock(string_view key, string &value, int64_t seq = INT64_MAX) -> RC;
  auto ForEachNoLock(std::function<RC(const MemKey &key, string_view value)> &&func) -> RC;
  auto BuildSSTable(string_view dbname, FileMetaData **meta_data_pointer) -> RC;
  auto GetMemTableSize() -> size_t;
  auto DropWAL() -> RC;
  auto Empty() -> bool;

 private:
  Stat                 stat_;
  std::unique_ptr<WAL> wal_;
  mutable std::mutex   mtx_;

  std::map<std::string, std::string> table_;
};
}  // namespace lsm_tree