#pragma once

#include <cassert>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "return_code.hh"

namespace lsm_tree {

inline const uint32_t RESTARTS_BLOCK_LEN = 32;

using std::shared_ptr;
using std::string;
using std::string_view;

class BlockWriter {
 public:
  BlockWriter() = default;
  auto Add(std::string_view key, std::string_view value) -> RC;
  auto Final(std::string &result) -> RC;
  auto EstimatedSize() -> size_t;
  void Reset();
  auto Empty() -> bool;

 private:
  uint32_t         entries_num_{0};
  std::vector<int> restarts_;
  std::string      last_key_;
  std::string      buffer_;
};

class BlockReader : public std::enable_shared_from_this<BlockReader> {
 public:
  class Iterator {
   public:
    explicit Iterator(shared_ptr<BlockReader> &&container, size_t restarts_block_idx = 0
                      /* size_t entries_idx = 0 */);

    Iterator()
        : restarts_block_idx_(0),
          entries_idx_(0),
          value_len_(0),
          unshared_key_len_(0),
          shared_key_len_(0),
          cur_entry_(nullptr),
          valid_(false) {}

    auto operator++() -> Iterator &;
    auto operator++(int) -> Iterator;

    auto operator<=>(Iterator &rhs) -> int {
      assert(container_ == rhs.container_);

      if (cur_entry_ > rhs.cur_entry_) {
        return 1;
      }
      if (cur_entry_ < rhs.cur_entry_) {
        return -1;
      }
      return 0;
    }

    auto operator=(const Iterator &rhs) -> Iterator &;

    Iterator(const Iterator &rhs);

    auto operator=(Iterator &&rhs) noexcept -> Iterator &;

    Iterator(Iterator &&rhs) noexcept;

    auto operator==(Iterator &rhs) -> bool { return container_ == rhs.container_ && cur_entry_ == rhs.cur_entry_; }

    auto operator!=(Iterator &rhs) -> bool { return !(*this == rhs); }

    auto Valid() -> bool { return valid_; }

    auto     GetContainer() -> shared_ptr<BlockReader> { return container_; }
    explicit operator bool() {
      if (!container_) {
        return false;
      }
      if (*this >= container_->End()) {
        return false;
      }

      return true;
    }

    void Fetch();
    void FetchWithoutValue();

    auto Key() const -> string_view { return cur_key_; }
    auto Value() const -> string_view { return cur_value_; }

   private:
    void SetInValid();

    size_t                  restarts_block_idx_;
    size_t                  entries_idx_;
    shared_ptr<BlockReader> container_;

    const char *cur_entry_;
    int         value_len_;
    int         unshared_key_len_;
    int         shared_key_len_;

    bool   valid_;
    string cur_key_;
    string cur_value_;
  };

  auto Begin() -> Iterator;
  auto End() -> Iterator;
  BlockReader() = default;
  auto Init() -> RC;
  auto Get(std::string_view want_key, std::string &key, std::string &value) -> RC;

 private:
  auto BsearchRestartPoint(string_view key, int *index) -> RC;
  auto GetInternal(string_view key, const std::function<RC(string_view, string_view)> &handle_result) -> RC;

  string_view data_;        /* [data][restarts] */
  string_view data_buffer_; /* [data] */
  /* 既是重启点数组的起点偏移量，也是数据项的结束偏移量 */
  size_t                                                                       restarts_offset_;
  std::vector<int>                                                             restarts_;  //  重启点
  std::function<int(string_view, string_view)>                                 cmp_fn_;
  std::function<RC(string_view, string_view, string_view, string &, string &)> handle_result_fn_;
};

// BlockMeta
//  ----------------------------
// | block_offset | block_size |
//  ----------------------------
// | 4 bytes      | 4 bytes    |
//  ----------------------------

struct BlockHandle {
  int block_offset_ = 0;
  int block_size_   = 0;

  void EncodeMeta(string &ret) {
    ret.append(reinterpret_cast<char *>(&block_offset_), sizeof(int));
    ret.append(reinterpret_cast<char *>(&block_size_), sizeof(int));
  }

  void DecodeFrom(string_view src) {
    block_offset_ = *reinterpret_cast<const int *>(src.data());
    block_size_   = *reinterpret_cast<const int *>(src.data() + sizeof(int));
  }

  void SetMeta(int block_offset, int block_size) {
    block_offset_ = block_offset;
    block_size_   = block_size;
  }
};

struct BlockCacheHandle {
  string oid_;
  int    offset_;
  BlockCacheHandle() = default;
  BlockCacheHandle(string oid, int offset) : oid_(std::move(oid)), offset_(offset) {}
  auto operator==(const BlockCacheHandle &h) const -> bool { return h.oid_ == oid_ && h.offset_ == offset_; }
};

auto DecodeRestartsPointKeyWrap(const char *restart_record, string_view &restarts_key) -> RC;
auto DecodeRestartsPointValueWrap(const char *restart_record, string_view &restarts_value) -> RC;

auto DecodeRestartsPointKeyAndValue(const char *restart_record, int *shared_key_len, int *unshared_key_len,
                                    int *value_len, string_view &restarts_key, string_view &restarts_value) -> RC;

auto DecodeRestartsPointKeyAndValueWrap(const char *restart_record, string_view &restarts_key,
                                        string_view &restarts_value) -> RC;

}  // namespace lsm_tree

namespace std {
template <>  // function-template-specialization
class hash<lsm_tree::BlockCacheHandle> {
 public:
  auto operator()(const lsm_tree::BlockCacheHandle &h) const -> size_t {
    return hash<string>()(h.oid_) ^ hash<int>()(h.offset_);
  }
};

};  // namespace std