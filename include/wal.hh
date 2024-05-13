/**
 * @file wal.hh
 * @author gusj (guchee@163.com)
 * @brief Write ahead log
 * @version 0.1
 * @date
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#include <memory>
#include <string>
#include <string_view>
#include "return_code.hh"
#include "util/file_util.hh"
namespace lsm_tree {

using std::unique_ptr;

enum WALDataType { wal_kv_ };
class WAL {
 public:
  explicit WAL(WritAbleFile *wal_file);
  virtual auto AddRecord(string_view data) -> RC;

  auto Sync() -> RC;
  auto Close() -> RC;
  auto Drop() -> RC;
  virtual ~WAL() = default;

 protected:
  /* 预写日志文件 */
  unique_ptr<WritAbleFile> wal_file_;
};

class SeqReadFile;
class WALReader {
 public:
  explicit WALReader(SeqReadFile *wal_file);
  auto ReadRecord(string &record) -> RC;
  auto Drop() -> RC;
  auto Close() -> RC;

 private:
  /* 预写日志文件 */
  unique_ptr<SeqReadFile> wal_file_;
};

}  // namespace lsm_tree