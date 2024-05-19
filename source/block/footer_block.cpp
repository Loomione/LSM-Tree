#include "block/footer_block.hh"

namespace lsm_tree {

auto FooterBlockWriter::Add(string_view meta_block_handle, string_view index_block_handle) -> RC {
  meta_block_handle_  = meta_block_handle;
  index_block_handle_ = index_block_handle;
  return RC::OK;
}

auto FooterBlockWriter::Final(string &result) -> RC {
  static const char magic_number[2] = {0x12, 0x34};
  string            footer;
  if (meta_block_handle_.length() != 8 || index_block_handle_.length() != 8) {
    return RC::UN_SUPPORTED_FORMAT;
  }
  footer.append(meta_block_handle_);
  footer.append(index_block_handle_);
  footer.append(magic_number, 2);
  if (FOOTER_SIZE != footer.length()) {
    return RC::UN_SUPPORTED_FORMAT;
  }
  result = std::move(footer);
  return RC::OK;
}

auto FooterBlockReader::Init(string_view footer_buffer) -> RC {
  footer_buffer_ = footer_buffer;
  if (footer_buffer_.length() != FooterBlockWriter::FOOTER_SIZE ||
      footer_buffer_[FooterBlockWriter::FOOTER_SIZE - 2] != 0x12 ||
      footer_buffer_[FooterBlockWriter::FOOTER_SIZE - 1] != 0x34) {
    return RC::UN_SUPPORTED_FORMAT;
  }
  meta_block_handle_.DecodeFrom(footer_buffer_.substr(0, 8));
  index_block_handle_.DecodeFrom(footer_buffer_.substr(8, 16));
  return RC::OK;
}
}  // namespace lsm_tree