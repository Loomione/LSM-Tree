/**
 * @file sstable.hh
 * @author gusj (guchee@163.com)
 * @brief
 * @version 0.1
 * @date 2024-07-03
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <memory>
#include <string>

#include "block/block.hh"
#include "block/filter_block.hh"
#include "block/footer_block.hh"
#include "options.hh"
#include "util/file_util.hh"

namespace lsm_tree {
class SSTableWriter {
 public:
  SSTableWriter(string_view dbname, WritAbleFile *file, const DBOptions &options);

 private:
  static constexpr unsigned int need_flush_size_ = (1UL << 12); /* 4KB */

  string                   dbname_;
  unique_ptr<WritAbleFile> file_;

  /* 数据块 */
  BlockWriter data_block_;
  BlockHandle data_block_handle_;

  /* 索引块 */
  BlockWriter index_block_;
  BlockHandle index_block_handle_;

  /* 过滤器块 */
  FilterBlockWriter filter_block_;
  BlockHandle       filter_block_handle_;

  /* 元数据块 */
  BlockWriter meta_data_block_;
  BlockHandle meta_data_block_handle_;

  /* 尾信息块 */
  FooterBlockWriter foot_block_;
  // BlockHandle foot_block_handle_;

  SHA256_CTX sha256_;
  string     buffer_;   /* 数据 */
  int        offset_;   /* 数据的偏移量 */
  string     last_key_; /* 最后一次 add 的 key */
};

class SSTableReader : public std::enable_shared_from_this<SSTableReader> {};
}  // namespace lsm_tree
