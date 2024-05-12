
/**
 * @file keys.hh
 * @author gusj (guchee@163.com)
 * @brief
 * @version 0.1
 * @date 2024-05-12
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once
#include <ostream>
#include <string>
#include <string_view>

#include "return_code.hh"

namespace lsm_tree {

enum class OperatorType {
  PUT,
  DELETE,
};
/*

inner_key = user_key + seq + type
__________________________
| user_key | seq | type |
-------------------------

user_key: 用于用户查询的key
seq:  用于区分相同user_key的不同版本, 大小为8字节
type: 操作类型, PUT/DELETE, 大小为1字节

*/
struct MemKey {
  std::string  user_key_;
  int64_t      seq_;
  OperatorType type_;

  MemKey()                                        = default;
  MemKey(const MemKey &other)                     = default;
  auto operator=(const MemKey &other) -> MemKey & = default;
  MemKey(MemKey &&other) noexcept : user_key_(std::move(other.user_key_)), seq_(other.seq_), type_(other.type_) {}
  auto operator=(MemKey &&other) noexcept -> MemKey & {
    if (this != &other) {
      user_key_ = std::move(other.user_key_);
      seq_      = other.seq_;
      type_     = other.type_;
    }
    return *this;
  }
  MemKey(std::string_view key, int64_t seq, OperatorType type) : user_key_(key), seq_(seq), type_(type) {}

  friend auto operator<<(std::ostream &os, const MemKey &key) -> std::ostream &;
  auto        operator<(const MemKey &other) const -> bool;
  auto        ToSSTableKey() const -> std::string;
  void        FromSSTableKey(std::string_view key);
  auto        Size() const -> size_t { return user_key_.size() + sizeof(seq_) + sizeof(type_); }
  static auto NewMinMemKey(std::string_view key) -> MemKey;
};

inline auto InnerKeyToUserKey(std::string_view inner_key) -> std::string_view;
inline auto InnerKeySeq(std::string_view inner_key) -> int64_t;
inline auto InnerKeyOpType(std::string_view inner_key) -> OperatorType;

auto CmpInnerKey(std::string_view k1, std::string_view k2) -> int;
auto CmpUserKeyOfInnerKey(std::string_view k1, std::string_view k2) -> int;
auto CmpKeyAndUserKey(std::string_view key, std::string_view user_key) -> int;
auto SaveResultIfUserKeyMatch(std::string_view rk, std::string_view rv, std::string_view tk, std::string &dk,
                              std::string &dv) -> RC;
auto NewMinInnerKey(std::string_view key) -> std::string;

auto EasyCmp(std::string_view key1, std::string_view key2) -> int;
auto EasySaveValue(std::string_view rk, std::string_view rv, std::string_view tk, std::string &dk, std::string &dv)
    -> RC;

auto EncodeKVPair(const MemKey &key, std::string_view value) -> std::string;
void DecodeKVPair(std::string_view data, MemKey &memkey, std::string &value);

}  // namespace lsm_tree