#include "block/block.hh"
#include <sys/types.h>
#include <cstdint>
#include "util/encode.hh"
#include "util/monitor_logger.hh"
namespace lsm_tree {

/*
**********************************************************************************************************************************************
* BlockWriter
**********************************************************************************************************************************************
*/

/**
 * @brief 向当前块中添加一个键值对。
 *
 * 此函数将一个键值对添加到当前块中，并使用前缀压缩技术来节省空间。
 * 它通过新的条目更新缓冲区，并维护重启点以便高效查找。
 * 如果条目数量达到预定义的阈值，将添加一个新的重启点。
 *
 * @param key 要添加的键。
 * @param value 与键相关联的值。
 * @return RC 如果操作成功，返回 RC::OK；否则返回错误码。
 */
auto BlockWriter::Add(std::string_view key, std::string_view value) -> RC {
  int value_len      = static_cast<int>(value.length());  // 获取值的长度
  int key_len        = static_cast<int>(key.length());    // 获取键的长度
  int shared_key_len = 0;                                 // 初始化共享前缀长度为0
  int unshared_key_len;                                   // 非共享前缀的长度

  if ((entries_num_ % RESTARTS_BLOCK_LEN) == 0) {
    restarts_.push_back(static_cast<uint32_t>(buffer_.size()));  // 添加重启点
  } else {
    auto min_len = std::min(key_len, static_cast<int>(last_key_.length()));  // 获取当前键和上一个键长度的最小值
    /* 寻找当前 key 和 last_key 的共享前缀长度 */
    for (int i = 0; i < min_len; ++i) {
      if (key[i] != last_key_[i]) {
        break;
      }
      shared_key_len++;  // 计算共享前缀长度
    }
  }

  unshared_key_len = key_len - shared_key_len;                               // 计算非共享前缀长度
  buffer_.append(reinterpret_cast<char *>(&shared_key_len), sizeof(int));    // 添加共享前缀长度到缓冲区
  buffer_.append(reinterpret_cast<char *>(&unshared_key_len), sizeof(int));  // 添加非共享前缀长度到缓冲区
  buffer_.append(reinterpret_cast<char *>(&value_len), sizeof(int));         // 添加值的长度到缓冲区
  buffer_.append(key.data() + shared_key_len, unshared_key_len);             // 添加非共享前缀的键到缓冲区
  buffer_.append(value.data(), value_len);                                   // 添加值到缓冲区

  entries_num_++;   // 增加键值对数量
  last_key_ = key;  // 更新 last_key_
  return RC::OK;    // 返回操作成功
}

/**
 * @brief 将当前块的内容输出到指定的字符串中，并在末尾添加重启点偏移量及其长度。
 *
 * @param result 输出参数，存储当前块的内容。
 * @return RC 如果操作成功，返回 RC::OK；否则返回错误码。
 */
auto BlockWriter::Final(string &result) -> RC {
  /* 添加重启点偏移量及其长度 */
  int restarts_len = static_cast<int>(restarts_.size());
  for (int i = 0; i < restarts_len; i++) {
    buffer_.append(reinterpret_cast<char *>(&restarts_[i]), sizeof(uint32_t));
  }
  buffer_.append(reinterpret_cast<char *>(&restarts_len), sizeof(uint32_t));
  result = std::move(buffer_);
  return RC::OK;
}

auto BlockWriter::EstimatedSize() -> size_t { return buffer_.size() + (restarts_.size() + 1) * sizeof(int); }

void BlockWriter::Reset() {
  restarts_.clear();
  last_key_.clear();
  entries_num_ = 0;
  buffer_.clear();
}

auto BlockWriter::Empty() -> bool { return entries_num_ == 0; }

/*
**********************************************************************************************************************************************
* BlockReader
**********************************************************************************************************************************************
*/

/**
 * @brief 初始化 BlockReader 对象，设置数据块、比较函数和处理结果函数。
 *
 * @param data 数据块，以 string_view 类型传入。
 * @param cmp 比较函数，用于比较两个 string_view 类型的字符串。
 * @param handle_result 处理结果函数，用于处理从数据块中提取的键和值。
 * @return RC 如果初始化成功，返回 RC::OK；否则返回错误码。
 */
auto BlockReader::Init(
    string_view data, std::function<int(string_view, string_view)> &&cmp,
    std::function<RC(string_view, string_view, string_view innner_key, string &key, string &value)> &&handle_result)
    -> RC {
  cmp_fn_              = std::move(cmp);
  handle_result_fn_    = std::move(handle_result);
  data_                = data;
  const char *buffer   = data_.data();
  size_t      data_len = data_.length();
  /* restarts_len */
  int    restarts_len;
  size_t restarts_len_offset = data_len - sizeof(uint32_t);
  Decode32(buffer + restarts_len_offset, &restarts_len);
  /* restarts */
  restarts_offset_ = restarts_len_offset - restarts_len * sizeof(uint32_t);
  for (int i = 0; i < restarts_len; i++) {
    int offset;
    Decode32(buffer + restarts_offset_ + i * sizeof(int), &offset);
    restarts_.push_back(offset);
  }
  /*data_buffer*/
  data_buffer_ = data.substr(0, restarts_offset_);
  return RC::OK;
}

/**
 * @brief 在重启点数组中找到恰好小于等于给定键的重启点索引
 *
 * @param[in] key 需要查找的目标键
 * @param[out] index 存储找到的重启点索引
 * @return RC 返回代码，表示操作是否成功
 */
auto BlockReader::BsearchRestartPoint(string_view key, int *index) -> RC {
  int         restarts_len = static_cast<int>(restarts_.size());
  int         left         = 0;
  int         right        = restarts_len - 1;
  int         value_len;
  int         unshared_key_len;
  int         shared_key_len;
  string_view restarts_key;
  auto        rc = RC::OK;
  /* 二分找到恰好小于等于 key 的重启点 */
  while (left <= right) {
    int         mid            = (left + right) >> 1;
    const char *restart_record = data_.data() + restarts_[mid];

    if (RC rc = DecodeRestartsPointKeyWrap(restart_record, restarts_key); rc != RC::OK) {
      MLog->error("DecodeRestartsPointKey error: {}", RcToString(rc));
      return rc;
    }
    if (cmp_fn_(restarts_key, key) < 0) {
      left = mid + 1;
    } else {
      right = mid - 1;
    }
  }

  /* 如果 left == restarts_len 则 index 为 right */
  if (left != restarts_len) {
    /* 否则看下 left 是否和 key 相等如果是则直接返回left */
    const char *restart_record = data_.data() + restarts_[left];
    if (rc = DecodeRestartsPointKeyWrap(restart_record, restarts_key); rc != RC::OK) {
      MLog->error("DecodeRestartsPointKey error: {}", RcToString(rc));
      return rc;
    }
    if (cmp_fn_(restarts_key, key) == 0) {
      *index = left;
      return RC::OK;
    }
  }

  *index = right;
  return RC::OK;
}

/**
 * @brief 查找给定的内部键，并通过回调函数处理结果
 *
 * @param inner_key 需要查找的内部键
 * @param handle_result 处理查找结果的回调函数
 * @return RC 返回代码，表示操作是否成功
 */
auto BlockReader::GetInternal(string_view inner_key, const std::function<RC(string_view, string_view)> &handle_result)
    -> RC {
  int                  restarts_len = static_cast<int>(restarts_.size());
  [[maybe_unused]] int key_len      = static_cast<int>(inner_key.length());
  int                  index        = 0;
  /* 二分查找最近的小于 key 的重启点 如果 key 越界了，那就直接返回没找到 */
  auto rc = BsearchRestartPoint(inner_key, &index);
  if (rc != RC::OK) {
    return rc;
  }
  if (index >= restarts_len) {
    return RC::NOT_FOUND;
  }
  if (index == -1) {
    const char *restart_record = data_.data() + restarts_[0];
    string_view key;
    string_view val;
    if (rc = DecodeRestartsPointKeyAndValueWrap(restart_record, key, val); rc != RC::OK) {
      return rc;
    }
    return handle_result(key, val);
  }

  /* 剩下的线性扫描 找到恰好大于等于 inner_key 的地方 */
  auto   cur_entry = data_.data() + restarts_[index];
  string last_key;
  int    i;

  for (i = 0; i < RESTARTS_BLOCK_LEN && cur_entry < data_buffer_.end(); i++) {
    int value_len;
    int unshared_key_len;
    int shared_key_len;
    Decode32(cur_entry, &shared_key_len);
    Decode32(cur_entry + sizeof(int), &unshared_key_len);
    Decode32(cur_entry + sizeof(int) * 2, &value_len);
    string cur_key;
    /* expect ok */
    if (last_key.length() >= shared_key_len && (shared_key_len != 0)) {
      cur_key = last_key.substr(0, shared_key_len);
    }
    cur_key.append(cur_entry + sizeof(int) * 3, static_cast<size_t>(unshared_key_len));
    int cmp = cmp_fn_(cur_key, inner_key);
    /* 恰好大于等于 （而非强制等于） */
    if (cmp >= 0) {
      return handle_result(cur_key, {cur_entry + sizeof(int) * 3 + unshared_key_len, static_cast<size_t>(value_len)});
    }
    cur_entry += sizeof(int) * 3 + unshared_key_len + value_len;

    last_key = std::move(cur_key);
  }

  /* 还没有越界 我们检测下一个重启点 */
  if (i == RESTARTS_BLOCK_LEN && cur_entry < data_buffer_.end()) {
    string_view key;
    string_view val;
    if (rc = DecodeRestartsPointKeyAndValueWrap(cur_entry, key, val); rc != RC::OK) {
      return rc;
    }
    return handle_result(key, val);
  }

  return RC::NOT_FOUND;
}

/*
**********************************************************************************************************************************************
* BlockReader::Iterator
**********************************************************************************************************************************************
*/

void BlockReader::Iterator::Fetch() {
  if (cur_entry_ >= container_->data_buffer_.end() || entries_idx_ >= RESTARTS_BLOCK_LEN ||
      restarts_block_idx_ >= container_->restarts_.size()) {
    return;
  }
  Decode32(cur_entry_, &shared_key_len_);
  Decode32(cur_entry_ + sizeof(int), &unshared_key_len_);
  Decode32(cur_entry_ + sizeof(int) * 2, &value_len_);
  if (cur_key_.length() >= shared_key_len_) {
    cur_key_ = cur_key_.substr(0, shared_key_len_);
  }
  cur_key_.append(cur_entry_ + sizeof(int) * 3, static_cast<size_t>(unshared_key_len_));
  cur_value_.append(cur_entry_ + sizeof(int) * 3 + unshared_key_len_, static_cast<size_t>(value_len_));
  valid_ = true;
}

void BlockReader::Iterator::FetchWithoutValue() {
  if (cur_entry_ >= container_->data_buffer_.end() || entries_idx_ >= RESTARTS_BLOCK_LEN ||
      restarts_block_idx_ >= container_->restarts_.size()) {
    return;
  }
  Decode32(cur_entry_, &shared_key_len_);
  Decode32(cur_entry_ + sizeof(int), &unshared_key_len_);
  Decode32(cur_entry_ + sizeof(int) * 2, &value_len_);
  if (cur_key_.length() >= shared_key_len_) {
    cur_key_ = cur_key_.substr(0, shared_key_len_);
  }
  cur_key_.append(cur_entry_ + sizeof(int) * 3, static_cast<size_t>(unshared_key_len_));
}

BlockReader::Iterator::Iterator(Iterator &&rhs) noexcept
    : container_(std::move(rhs.container_)),
      restarts_block_idx_(rhs.restarts_block_idx_),
      entries_idx_(rhs.entries_idx_),
      cur_entry_(rhs.cur_entry_),
      cur_key_(std::move(rhs.cur_key_)),
      cur_value_(std::move(rhs.cur_value_)),
      value_len_(rhs.value_len_),
      unshared_key_len_(rhs.unshared_key_len_),
      shared_key_len_(rhs.shared_key_len_),
      valid_(rhs.valid_) {}

auto BlockReader::Iterator::operator=(Iterator &&rhs) noexcept -> BlockReader::Iterator & {
  if (&rhs != this) {
    restarts_block_idx_ = rhs.restarts_block_idx_;
    entries_idx_        = rhs.entries_idx_;
    container_          = std::move(rhs.container_);
    cur_entry_          = rhs.cur_entry_;
    cur_key_            = std::move(rhs.cur_key_);
    cur_value_          = std::move(rhs.cur_value_);
    value_len_          = rhs.value_len_;
    unshared_key_len_   = rhs.unshared_key_len_;
    shared_key_len_     = rhs.shared_key_len_;
    valid_              = rhs.valid_;
  }
  return *this;
}
BlockReader::Iterator::Iterator(const Iterator &rhs) = default;

auto BlockReader::Iterator::operator=(const Iterator &rhs) -> BlockReader::Iterator & {
  if (&rhs != this) {
    restarts_block_idx_ = rhs.restarts_block_idx_;
    entries_idx_        = rhs.entries_idx_;
    container_          = rhs.container_;
    cur_entry_          = rhs.cur_entry_;
    cur_key_            = rhs.cur_key_;
    cur_value_          = rhs.cur_value_;
    value_len_          = rhs.value_len_;
    unshared_key_len_   = rhs.unshared_key_len_;
    shared_key_len_     = rhs.shared_key_len_;
    valid_              = rhs.valid_;
  }
  return *this;
}
auto BlockReader::Iterator::operator++(int) -> BlockReader::Iterator {
  Iterator tmp = *this;
  this->   operator++();
  return tmp;
}

auto BlockReader::Iterator::operator++() -> BlockReader::Iterator & {
  if (!valid_) {
    FetchWithoutValue();
  }
  valid_ = false;
  cur_value_.clear();
  if (cur_entry_ >= container_->data_buffer_.end()) {
    return *this;
  }
  if (entries_idx_ + 1 < RESTARTS_BLOCK_LEN) {
    entries_idx_++;

  } else if (entries_idx_ + 1 == RESTARTS_BLOCK_LEN && restarts_block_idx_ < container_->restarts_.size()) {
    restarts_block_idx_++;
    entries_idx_ = 0;
  }
  cur_entry_ += sizeof(int) * 3 + unshared_key_len_ + value_len_;
  return *this;
}

BlockReader::Iterator::Iterator(shared_ptr<BlockReader> &&container,
                                size_t                    restarts_block_idx /* , size_t entries_idx */)
    : container_(std::move(container)), restarts_block_idx_(restarts_block_idx), entries_idx_(0), valid_(false) {
  if (restarts_block_idx_ >= container_->restarts_.size()) {
    SetInValid();
    return;
  }

  cur_entry_ = container_->data_.data() + container_->restarts_[restarts_block_idx];

  if (cur_entry_ > container_->data_buffer_.end()) {
    SetInValid();
  }
}

void BlockReader::Iterator::SetInValid() {
  auto restarts_len   = container_->restarts_.size();
  cur_entry_          = container_->data_buffer_.end();
  restarts_block_idx_ = restarts_len;
  entries_idx_        = 0;
  shared_key_len_     = 0;
  unshared_key_len_   = 0;
  value_len_          = 0;
}

/*
***********************************************************************
* Decode Functions
***********************************************************************
*/

auto DecodeRestartsPointKeyAndValue(const char *restart_record, int *shared_key_len, int *unshared_key_len,
                                    int *value_len, string_view &restarts_key, string_view &restarts_value) -> RC {
  Decode32(restart_record, shared_key_len);
  if (*shared_key_len != 0) {
    return RC::UN_SUPPORTED_FORMAT;
  }
  Decode32(restart_record + sizeof(int), unshared_key_len);
  Decode32(restart_record + sizeof(int) * 2, value_len);
  restarts_key   = {restart_record + sizeof(int) * 3, static_cast<size_t>(*unshared_key_len)};
  restarts_value = {restart_record + sizeof(int) * 3 + *unshared_key_len, static_cast<size_t>(*value_len)};
  return RC::OK;
}

auto DecodeRestartsPointKeyAndValueWrap(const char *restart_record, string_view &restarts_key,
                                        string_view &restarts_value) -> RC {
  int shared_key_len;
  int unshared_key_len;
  int value_len;
  return DecodeRestartsPointKeyAndValue(restart_record, &shared_key_len, &unshared_key_len, &value_len, restarts_key,
                                        restarts_value);
}

auto DecodeRestartsPointKeyWrap(const char *restart_record, string_view &restarts_key) -> RC {
  string_view unused_restarts_value;
  return DecodeRestartsPointKeyAndValueWrap(restart_record, restarts_key, unused_restarts_value);
}

auto DecodeRestartsPointValueWrap(const char *restart_record, string_view &restarts_value) -> RC {
  string_view unused_restarts_key;
  return DecodeRestartsPointKeyAndValueWrap(restart_record, unused_restarts_key, restarts_value);
}

}  // namespace lsm_tree
