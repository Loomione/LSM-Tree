/**
 * @file rc.hh
 * @author gusj (guchee@163.com)
 * @brief  Return code
 * @version 0.1
 * @date
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once
#include <string_view>

namespace lsm_tree {
enum class RC {
  OK,
  NOT_FOUND,
  IS_NOT_DIRECTORY,
  CREATE_DIRECTORY_FAILED,
  DESTROY_DIRECTORY_FAILED,
  DESTROY_FILE_FAILED,
  UN_IMPLEMENTED,
  EXISTED,
  OPEN_FILE_ERROR,
  IO_ERROR,
  CLOSE_FILE_ERROR,
  RENAME_FILE_ERROR,
  MAKESTEMP_ERROR,
  FILTER_BLOCK_ERROR,
  FOOTER_BLOCK_ERROR,
  UN_SUPPORTED_FORMAT,
  DB_CLOSED,
  STAT_FILE_ERROR,
  MMAP_ERROR,
  OUT_OF_RANGE,
  BAD_LEVEL,
  BAD_REVISION,
  BAD_FILE_META,
  BAD_RECORD,
  FILE_EOF,
  CHECK_SUM_ERROR,
  NOEXCEPT_SIZE,
  BAD_FILE_PATH,
  BAD_CURRENT_FILE,
  NEW_SSTABLE_ERROR,
  CREATE_FILE_FAILED,
};

inline auto RcToString(RC rc) -> std::string_view {
  switch (rc) {
    case RC::OK:
      return "OK";
    case RC::NOT_FOUND:
      return "NOT_FOUND";
    case RC::IS_NOT_DIRECTORY:
      return "IS_NOT_DIRECTORY";
    case RC::CREATE_DIRECTORY_FAILED:
      return "CREATE_DIRECTORY_FAILED";
    case RC::DESTROY_DIRECTORY_FAILED:
      return "DESTROY_DIRECTORY_FAILED";
    case RC::DESTROY_FILE_FAILED:
      return "DESTROY_FILE_FAILED";
    case RC::UN_IMPLEMENTED:
      return "UN_IMPLEMENTED";
    case RC::EXISTED:
      return "EXISTED";
    case RC::OPEN_FILE_ERROR:
      return "OPEN_FILE_ERROR";
    case RC::IO_ERROR:
      return "IO_ERROR";
    case RC::CLOSE_FILE_ERROR:
      return "CLOSE_FILE_ERROR";
    case RC::RENAME_FILE_ERROR:
      return "RENAME_FILE_ERROR";
    case RC::MAKESTEMP_ERROR:
      return "MAKESTEMP_ERROR";
    case RC::FILTER_BLOCK_ERROR:
      return "FILTER_BLOCK_ERROR";
    case RC::FOOTER_BLOCK_ERROR:
      return "FOOTER_BLOCK_ERROR";
    case RC::UN_SUPPORTED_FORMAT:
      return "UN_SUPPORTED_FORMAT";
    case RC::DB_CLOSED:
      return "DB_CLOSED";
    case RC::STAT_FILE_ERROR:
      return "STAT_FILE_ERROR";
    case RC::MMAP_ERROR:
      return "MMAP_ERROR";
    case RC::OUT_OF_RANGE:
      return "OUT_OF_RANGE";
    case RC::BAD_LEVEL:
      return "BAD_LEVEL";
    case RC::BAD_REVISION:
      return "BAD_REVISION";
    case RC::BAD_FILE_META:
      return "BAD_FILE_META";
    case RC::BAD_RECORD:
      return "BAD_RECORD";
    case RC::FILE_EOF:
      return "FILE_EOF";
    case RC::CHECK_SUM_ERROR:
      return "CHECK_SUM_ERROR";
    case RC::NOEXCEPT_SIZE:
      return "NOEXCEPT_SIZE";
    case RC::BAD_FILE_PATH:
      return "BAD_FILE_PATH";
    case RC::BAD_CURRENT_FILE:
      return "BAD_CURRENT_FILE";
    case RC::NEW_SSTABLE_ERROR:
      return "NEW_SSTABLE_ERROR";
    case RC::CREATE_FILE_FAILED:
      return "CREATE_FILE_FAILED";
  }
  return "UNKNOWN";
}
}  // namespace lsm_tree