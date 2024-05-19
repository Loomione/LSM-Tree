#pragma once

#include "block.hh"
#include "return_code.hh"

namespace lsm_tree {

class FooterBlockWriter {
 public:
  auto Add(string_view meta_block_handle, string_view index_block_handle) -> RC;
  auto Final(string &result) -> RC;

  static constexpr int FOOTER_SIZE = 2 + 8 * 2;

 private:
  string_view meta_block_handle_;
  string_view index_block_handle_;
};

class FooterBlockReader {
 public:
  FooterBlockReader() = default;
  auto Init(string_view footer_buffer) -> RC;
  auto MetaBlockHandle() const -> const BlockHandle & { return meta_block_handle_; }
  auto IndexBlockHandle() const -> const BlockHandle & { return index_block_handle_; }

 private:
  string_view footer_buffer_;
  BlockHandle meta_block_handle_;
  BlockHandle index_block_handle_;
};

}  // namespace lsm_tree