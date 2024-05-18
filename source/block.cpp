#include "block.hh"

namespace lsm_tree {

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
    restarts_.push_back(static_cast<int>(buffer_.size()));  // 添加重启点
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
    buffer_.append(reinterpret_cast<char *>(&restarts_[i]), sizeof(int));
  }
  buffer_.append(reinterpret_cast<char *>(&restarts_len), sizeof(int));
  result = std::move(buffer_);
  return RC::OK;
}

auto BlockWriter::EstimatedSize() -> size_t { return buffer_.size() + (restarts_.size() + 1) * sizeof(int); }

void BlockWriter::Reset() {
  restarts_.clear();
  last_key_.clear();
  entries_num_ = 0;
  buffer_.clear();
};

auto BlockWriter::Empty() -> bool { return entries_num_ == 0; }

}  // namespace lsm_tree