#include "wal.hh"
#include <bits/types/FILE.h>
#include "crc32c/crc32c.h"
namespace lsm_tree {

/*
**********************************************************************************************************************************************
* WAL
**********************************************************************************************************************************************
*/

WAL::WAL(WritAbleFile *wal_file) { wal_file_.reset(wal_file); }

auto WAL::AddRecord(string_view data) -> RC {
  RC rc = RC::OK;
  /* checksum */
  uint32_t check_sum = crc32c::Crc32c(data);
  if (rc = wal_file_->Append({reinterpret_cast<char *>(&check_sum), sizeof(uint32_t)}); rc != RC::OK) {
    return rc;
  }
  /* type */
  int type = wal_kv_;
  if (rc = wal_file_->Append({reinterpret_cast<char *>(&type), sizeof(int)}); rc != RC::OK) {
    return rc;
  }
  /* len */
  int lens = static_cast<int>(data.length());
  if (rc = wal_file_->Append({reinterpret_cast<char *>(&lens), sizeof(int)}); rc != RC::OK) {
    return rc;
  }
  /* data */
  if (rc = wal_file_->Append(data); rc != RC::OK) {
    return rc;
  }
  return rc;
}

auto WAL::Sync() -> RC {
  if (RC rc = wal_file_->Flush(); rc != RC::OK) {
    return rc;
  }
  return wal_file_->Sync();
}

auto WAL::Close() -> RC { return wal_file_->Close(); }

auto WAL::Drop() -> RC {
  // MLog->info("Drop WAL file {}", wal_file_->GetPath());
  return FileManager::Destroy(wal_file_->GetPath());
}

/*
**********************************************************************************************************************************************
* WALReader
**********************************************************************************************************************************************
*/

WALReader::WALReader(SeqReadFile *wal_file) { wal_file_.reset(wal_file); }

auto WALReader::ReadRecord(string &record) -> RC {
  RC          rc{RC::OK};
  string      buffer;
  string_view view;
  uint32_t    check_sum;
  int         type;
  int         lens;

  /* read head */
  if (rc = wal_file_->Read(sizeof(uint32_t) * 3, buffer, view); rc != RC::OK) {
    // MLog->error("read WAL head error");
    return rc;
  }
  if (view.length() != sizeof(uint32_t) * 3) {
    return RC::FILE_EOF;
  }
  /* checksum */
  memcpy(&check_sum, view.data(), sizeof(uint32_t));
  /* type */
  memcpy(&type, view.data() + sizeof(uint32_t), sizeof(int));
  if (type != wal_kv_) {
    // MLog->error("read WAL type error");
    return RC::BAD_RECORD;
  }
  /* len */
  memcpy(&lens, view.data() + sizeof(uint32_t) * 2, sizeof(int));
  /* data */
  if (rc = wal_file_->Read(lens, buffer, view); rc != RC::OK) {
    // MLog->error("read WAL data error");
    return rc;
  }
  if (view.size() != lens) {
    return RC::FILE_EOF;
  }
  if (check_sum != crc32c::Crc32c(view)) {
    // MLog->error("check sum error");
    return RC::CHECK_SUM_ERROR;
  }
  record.assign(view.data(), view.size());
  return rc;
}

auto WALReader::Drop() -> RC {
  //   MLog->info("Drop WAL file {}", wal_file_->GetPath());
  return FileManager::Destroy(wal_file_->GetPath());
}

auto WALReader::Close() -> RC { return wal_file_->Close(); }

}  // namespace lsm_tree