#include "block/filter_block.hh"
#include <cstdint>
#include "util/encode.hh"
#include "util/monitor_logger.hh"
#include "util/murmur3_hash.hh"

namespace lsm_tree {

/*
**********************************************************************************************************************************************
* BloomFilter
**********************************************************************************************************************************************
*/

/**
 * @brief Construct a new Bloom Filter:: Bloom Filter object
 *
 * @param bits_per_key
 */
BloomFilter::BloomFilter(int bits_per_key) : bits_per_key_(bits_per_key) {
  k_ = static_cast<int>(bits_per_key * 0.69);  // 0.69 =~ ln(2)
  if (k_ < 1) {
    k_ = 1;
  }
  if (k_ > 30) {
    k_ = 30;
  }
}

/**
 * @brief 将键集合转换为布隆过滤器的位图表示
 * @details 位图表示的格式如下：
 * | bitmap1 | bitmap2 | ... | bitmapn |
 * @param[in] keys 要添加到布隆过滤器的键集合
 * @param[out] result 存储生成的位图的字符串
 * @return RC 返回操作的状态码
 */
auto BloomFilter::Keys2Block(const vector<string> &keys, string &result) -> RC {
  auto     keys_len        = static_cast<uint32_t>(keys.size());
  uint32_t bitmap_bits_len = (keys_len * bits_per_key_ + 7) * 8;  // bitmap 长度（bits）
  uint32_t bitmap_len      = bitmap_bits_len / 8;                 // bitmap 长度（bytes）
  auto     init_len        = static_cast<uint32_t>(result.size());

  result.resize(init_len + bitmap_len);  // 开辟 bitmap 空间
  auto bitmap = &result[init_len];
  for (const auto &key : keys) {
    /* 双哈希模拟多哈希 （ leveldb 单哈希模拟多哈希）*/
    auto h1 = Murmur3Hash(0xe2c6928a, key.data(), key.size());
    auto h2 = Murmur3Hash(0xbaea8a8f, key.data(), key.size());
    for (int j = 0; j < k_; j++) {
      int bit_pos = static_cast<int>((h1 + j * h2) % bitmap_bits_len);
      bitmap[bit_pos / 8] |= (1 << (bit_pos % 8));
    }
  }
  return RC::OK;
}

/**
 * @brief 检查给定的键是否存在于布隆过滤器中
 *
 * @param key 要检查的键
 * @param bitmap 布隆过滤器的位图表示
 * @return true 键存可能在于布隆过滤器中
 * @return false 键不存在于布隆过滤器中
 */
auto BloomFilter::IsKeyExists(string_view key, string_view bitmap) -> bool {
  auto bitmap_bits_len = static_cast<uint32_t>(bitmap.size()) * 8;

  auto h1 = Murmur3Hash(0xe2c6928a, key.data(), key.size());
  auto h2 = Murmur3Hash(0xbaea8a8f, key.data(), key.size());
  for (int j = 0; j < k_; j++) {
    int bit_pos = static_cast<int>((h1 + j * h2) % bitmap_bits_len);
    if ((bitmap[bit_pos / 8] & (1 << (bit_pos % 8))) == 0) {
      return false;
    }
  }
  return true;
}

void BloomFilter::FilterInfo(string &info) {
  info.append("bf:");
  info.append(reinterpret_cast<char *>(&bits_per_key_), sizeof(int));
}

/*
**********************************************************************************************************************************************
* FilterBlockWriter
**********************************************************************************************************************************************
*/

/**
 * @brief Construct a new Filter Block Writer:: Filter Block Writer object
 *
 * @param method
 */
FilterBlockWriter::FilterBlockWriter(unique_ptr<FilterAlgorithm> &&method) : method_(std::move(method)) {}

/**
 * @brief 更新过滤器块，添加新的键
 *
 * @param key 要添加的键
 * @return RC 操作结果代码
 */
auto FilterBlockWriter::Update(string_view key) -> RC {
  keys_.emplace_back(key);
  return RC::OK;
}

/**
 * @brief 将当前存储的键转换为过滤器块
 *
 * @return RC 操作结果代码
 */
auto FilterBlockWriter::Keys2Block() -> RC {
  offsets_.push_back(static_cast<int>(buffer_.size()));
  method_->Keys2Block(keys_, buffer_);
  keys_.clear();
  return RC::OK;
}

/**
 * @brief 生成最终的过滤器块并返回结果
 * @details 生成的过滤器块的格式如下：
 * | bitmap | offsets | offset_begin_offset | offsets_len | filter_info | filter_info_len |
 * @param result 最终位图结果字符串
 * @return RC 操作结果代码
 */
auto FilterBlockWriter::Final(string &result) -> RC {
  if (!keys_.empty()) {
    Keys2Block();
  }

  int offset_begin_offset = static_cast<int>(buffer_.size());
  int offset_len          = static_cast<int>(offsets_.size());
  /* 追加 offsets */
  for (int i = 0; i < offset_len; i++) {
    buffer_.append(reinterpret_cast<char *>(&offsets_[i]), sizeof(int));
  }
  /* 追加 offsets[0].offset */
  buffer_.append(reinterpret_cast<char *>(&(offset_begin_offset)), sizeof(int));
  /* 追加  offsets_.size() */
  buffer_.append(reinterpret_cast<char *>(&(offset_len)), sizeof(int));

  /* 追加 filter 算法相关信息及其长度, 比如我们在 bloom filter 选择的 bits_per_key  */
  string filter_info;
  method_->FilterInfo(filter_info);
  int filter_info_len = static_cast<int>(filter_info.length());
  if (filter_info_len != 0) {
    buffer_.append(filter_info);
    buffer_.append(reinterpret_cast<char *>(&filter_info_len), sizeof(int));
  }
  result = std::move(buffer_);
  return RC::OK;
}

/*
***********************************************************************
* FilterBlockReader
***********************************************************************
*/

/**
 * @brief Construct a new Filter Block Reader:: Filter Block Reader object
 *
 */
FilterBlockReader::FilterBlockReader() : filters_nums_(0) {}

/**
 * @brief  初始化过滤器块
 * @details 生成的过滤器块的格式如下：
 * | bitmap | offsets | offset_begin_offset | offsets_len | filter_info | filter_info_len |
 * @param filter_blocks
 * @return RC
 */
auto FilterBlockReader::Init(string_view filter_blocks) -> RC {
  filter_blocks_        = filter_blocks;
  auto filter_block_len = filter_blocks_.length();
  /* 1. GET INFO_LEN */
  if (filter_block_len < static_cast<int>(sizeof(int))) {
    return RC::FILTER_BLOCK_ERROR;
  }
  int info_len        = 0;
  int info_len_offset = static_cast<int>(filter_block_len - sizeof(int));
  Decode32(&filter_blocks_[info_len_offset], &info_len);
  if (info_len > info_len_offset || info_len <= 0) {
    return RC::FILTER_BLOCK_ERROR;
  }
  /* 2. 根据 info 生成相应的过滤器算法。 */
  int info_offset = (info_len_offset - info_len);
  filter_info_    = {&filter_blocks_[info_offset], static_cast<size_t>(info_len)};
  auto rc         = CreateFilterAlgorithm();
  if (rc != RC::OK) {
    return rc;
  }
  /* 3. GET filter num */
  if (info_offset < sizeof(int)) {
    return RC::FILTER_BLOCK_ERROR;
  }
  int filters_num_offset = static_cast<int>(info_offset - sizeof(int));
  Decode32(&filter_blocks_[filters_num_offset], &filters_nums_);
  /* 4. GET filter 0 offset offset*/
  filters_offsets_offset_ = 0;
  if (filters_num_offset < sizeof(int)) {
    return RC::FILTER_BLOCK_ERROR;
  }
  int filters_offsets_offset_offset = static_cast<int>(filters_num_offset - sizeof(int));
  Decode32(&filter_blocks_[filters_offsets_offset_offset], &filters_offsets_offset_);
  /* 5. GET filter 0 offset*/
  if (filters_offsets_offset_ < 0) {
    return RC::FILTER_BLOCK_ERROR;
  }
  /* 6. GET filter 0 block addr */
  int filters_zero_offset = 0;
  Decode32(&filter_blocks_[filters_offsets_offset_], &filters_zero_offset);
  if (filters_zero_offset != 0) {
    return RC::FILTER_BLOCK_ERROR;
  }
  filters_offsets_ = {&filter_blocks_[filters_offsets_offset_], sizeof(int) * filters_nums_};
  MLog->info(
      "FilterBlockReader filter_block_len:{}, filters_nums_:{}, "
      "filters_offsets_offset_:{}, filters_zero_offset:{}",
      filter_block_len, filters_nums_, filters_offsets_offset_, filters_zero_offset);

  return RC::OK;
}

/* 目前只有 bf: */
auto FilterBlockReader::CreateFilterAlgorithm() -> RC {
  static const char *k_bloom_filter = "bf";
  string_view        type           = filter_info_.substr(0, 2);
  if (type != k_bloom_filter) {
    return RC::FILTER_BLOCK_ERROR;
  }
  int bits_per_key = 0;
  Decode32(&filter_info_[3], &bits_per_key);
  MLog->info("FilterBlockReader Use BloomFilter algorithm, bits_per_key:{}", bits_per_key);
  method_ = std::make_unique<BloomFilter>(bits_per_key);
  return RC::OK;
}

/**
 * @brief 检查给定的键是否存在于指定的过滤块中
 *
 * @param filter_block_num 要检查的过滤块的编号
 * @param key 要检查的键
 * @return true 键存在于过滤块中
 * @return false 键不存在于过滤块中
 */
auto FilterBlockReader::IsKeyExists(int filter_block_num, string_view key) -> bool {
  int filter_offset1;
  int filter_offset2;
  if (filter_block_num >= filters_nums_) {
    return false;
  }
  Decode32(&filters_offsets_[filter_block_num * sizeof(int)], &filter_offset1);
  if (filter_block_num + 1 == filters_nums_) {
    filter_offset2 = filters_offsets_offset_;
  } else {
    Decode32(&filters_offsets_[(filter_block_num + 1) * sizeof(int)], &filter_offset2);
  }
  return method_->IsKeyExists(key, {&filter_blocks_[filter_offset1], &filter_blocks_[filter_offset2]});
}
}  // namespace lsm_tree
