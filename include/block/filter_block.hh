#pragma once

#include <memory>
#include <vector>
#include "return_code.hh"

namespace lsm_tree {

using std::string;
using std::string_view;
using std::unique_ptr;
using std::vector;

class FilterAlgorithm {
 public:
  virtual auto Keys2Block(const vector<string> &keys, string &result) -> RC = 0;
  virtual auto IsKeyExists(string_view key, string_view bitmap) -> bool     = 0;
  virtual void FilterInfo(string &info);
  virtual ~FilterAlgorithm() = default;
};

class BloomFilter : public FilterAlgorithm {
 public:
  /* bits_per_key 将会决定 一个 bloom-filter-block n 个 key 需要存储的总大小 */
  explicit BloomFilter(int bits_per_key);
  auto Keys2Block(const vector<string> &keys, string &result) -> RC override;
  auto IsKeyExists(string_view key, string_view bitmap) -> bool override;
  void FilterInfo(string &info) override;
  ~BloomFilter() override = default;

 private:
  int bits_per_key_;  // 每个 key 所占用 的 bit 数量
  int k_;             // 哈希函数个数
};

/*
----------------------------------------------------------------------------------------
| bitmap1 | bitmap2 | ... | bitmapn |    offset1   |    offset2   | ... |    offsetn   |
----------------------------------------------------------------------------------------
|        offset_begin_offset        |                    offsets_len                   |
----------------------------------------------------------------------------------------
|             filter_info           |               filter_info_len                    |
----------------------------------------------------------------------------------------
*/

class FilterBlockWriter {
 public:
  explicit FilterBlockWriter(unique_ptr<FilterAlgorithm> &&method);
  auto Update(string_view key) -> RC;
  auto Final(string &result) -> RC;
  auto Keys2Block() -> RC;

 private:
  string         buffer_;   // filter_block 缓冲区，保存了多个位图，一个位图对应一个block
  vector<string> keys_;     // 用来保存目前填入的 key ，在 Keys2Block 被调用时生成filter_block
  vector<int>    offsets_;  // 每个 filter 的偏移量
  unique_ptr<FilterAlgorithm> method_;  // 过滤器算法，目前只有 bloom-filter
};

class FilterBlockReader {
 public:
  FilterBlockReader();
  auto Init(string_view filter_block) -> RC;
  auto IsKeyExists(int filter_block_num, string_view key) -> bool;

 private:
  auto                        CreateFilterAlgorithm() -> RC;
  int                         filters_nums_;            // 过滤器块的个数
  int                         filters_offsets_offset_;  // 过滤器偏移量数组在块中的偏移量
  string_view                 filters_offsets_;         // 过滤器数组
  string_view                 filter_info_;             // 过滤器信息
  string_view                 filter_blocks_;           // 整个过滤器块
  unique_ptr<FilterAlgorithm> method_;                  // 过滤器算法，目前只有 bloom-filter
};
}  // namespace lsm_tree