/**
 * @file memtable.cpp
 * @author gusj (guchee@163.com)
 * @brief
 * @version 0.1
 * @date 2024-05-16
 *
 * @copyright Copyright (c) 2024
 *
 */

#include "memtable/memtable.hh"
#include <mutex>
#include <shared_mutex>
#include "util/monitor_logger.hh"

namespace lsm_tree {

MemTable::MemTable(const DBOptions &options) : options_(&options) {}
MemTable::MemTable(const DBOptions &options, WAL *wal) : options_(&options) { wal_.reset(wal); }

auto MemTable::Empty() -> bool {
  std::shared_lock lock(mtx_);
  return table_.empty();
}

auto MemTable::Put(const MemKey &key, string_view value) -> RC {
  std::unique_lock lock(mtx_);
  if (key.type_ == OperatorType::PUT) {
    table_[key] = value;
  } else {
    table_[key] = "";
  }
  stat_.Update(key.Size(), value.size());
  return RC::OK;
}

auto MemTable::PutTeeWAL(const MemKey &key, string_view value) -> RC {
  auto rc = RC::OK;
  if (rc = wal_->AddRecord(EncodeKVPair(key, value)); rc != RC::OK) {
    return rc;
  }
  if (options_->sync_) {
    if (rc = wal_->Sync(); rc != RC::OK) {
      return rc;
    }
  }
  return Put(key, value);
}

/* 暂时用不加锁版本 */
auto MemTable::ForEachNoLock(std::function<RC(const MemKey &key, string_view value)> &&func) -> RC {
  for (auto &iter : table_) {
    const MemKey &memkey = iter.first;
    string_view   value  = iter.second;
    auto          rc     = func(memkey, value);
    if (rc != RC::OK) {
      return rc;
    }
  }
  return RC::OK;
}

/* 一般是 IMEMTABLE 进行 BUILD 不需要加锁 */
auto MemTable::BuildSSTable(string_view dbname, FileMetaData **meta_data_pointer) -> RC {
    // TODO (gsj)
    return RC::OK;
}

auto MemTable::GetMemTableSize() -> size_t {
  std::shared_lock lock(mtx_);
  return stat_.Sum();
}

auto MemTable::DropWAL() -> RC {
  auto rc = RC::OK;
  if (wal_) {
    MLog->trace("MemTable::DropWAL");
    if (rc = wal_->Sync(); rc != RC::OK) {
      return rc;
    }
    if (rc = wal_->Close(); rc != RC::OK) {
      return rc;
    }
    if (rc = wal_->Drop(); rc != RC::OK) {
      return rc;
    }
    wal_.reset();
  }
  return rc;
}

auto MemTable::Get(string_view key, string &value, int64_t seq) -> RC {
  std::unique_lock lock(mtx_);
  return GetNoLock(key, value);
}

auto MemTable::GetNoLock(string_view key, string &value, int64_t seq) -> RC {
  MemKey look_key(key, seq);
  auto   iter = table_.lower_bound(look_key);
  if (iter == table_.end()) {
    return RC::NOT_FOUND;
  }
  if (key == iter->first.user_key_ && iter->first.type_ != OperatorType::DELETE) {
    value = iter->second;
    return RC::OK;
  }
  return RC::NOT_FOUND;
}

}  // namespace lsm_tree