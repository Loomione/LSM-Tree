#pragma once

#include <cstddef>
#include "spdlog/spdlog.h"

namespace lsm_tree {

struct DBOptions {
  /* DB OPERATION */
  bool create_if_not_exists_ = false;

  /* SSTABLE */
  /* 布隆过滤器 */
  int bits_per_key_ = 10;

  /* MEMTABLE */
  /* 内存表最大大小，超过了则应该冻结内存表 */
  static constexpr size_t MEM_TABLE_MAX_SIZE = 1UL << 22; /* 4MB */
  static constexpr size_t BLOCK_CACHE_SIZE   = 1UL << 11; /* 2048 个 BLOCK */

  /* BACKGROUND */
  int background_workers_number_ = 1;

  /* LOG */
  const char *log_pattern_   = "[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] %v";
  const char *logger_name_   = "monitor_logger";
  const char *log_file_name_ = "monitor.log";

  spdlog::level::level_enum log_level_ = spdlog::level::err;

  /* sync */
  bool sync_ = false;

  /* major compaction */
  int level_files_limit_ = 4;
};
}  // namespace lsm_tree
