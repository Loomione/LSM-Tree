#include "keys.hh"
#include <fmt/format.h>
#include <cstring>
#include "return_code.hh"

namespace lsm_tree {
auto operator<<(std::ostream &os, const MemKey &key) -> std::ostream & {
  return os << fmt::format("@MemKey [user_key_:{} seq_:{} op_type_:{}]", key.user_key_, key.seq_,
                           key.type_ == OperatorType::DELETE ? "OP_DELETE" : "OP_PUT");
}

/**
 * @brief  重载<操作符
 * 首先比较user_key, 如果相等, 则比较seq, 如果seq相等, 则比较op
 *  序列号大的比较小吗
 * @param other
 * @return true
 * @return false
 */
auto MemKey::operator<(const MemKey &other) const -> bool {
  if (int ret = user_key_.compare(other.user_key_); ret) {
    return ret < 0;
  }
  if (seq_ == other.seq_) {
    return type_ == OperatorType::DELETE;
  }
  return seq_ > other.seq_;
}

/**
 * @brief  从sstable中的key中解析出user_key, seq, type
 *
 * @return std::string
 */
auto MemKey::ToSSTableKey() const -> std::string {
  return user_key_ + std::string(reinterpret_cast<const char *>(&seq_), sizeof(seq_)) +
         std::string(1, static_cast<char>(type_));
}

/**
 * @brief  从sstable中的key中解析出user_key, seq, type
 *
 * @param key
 */

void MemKey::FromSSTableKey(std::string_view key) {
  user_key_ = InnerKeyToUserKey(key);
  seq_      = InnerKeySeq(key);
  type_     = InnerKeyOpType(key);
}

auto MemKey::NewMinMemKey(std::string_view key) -> MemKey { return {key, 0, OperatorType::PUT}; }

/**
 * @brief 获取inner_key中的user_key
 *
 * @param inner_key
 * @return std::string_view
 */
auto InnerKeyToUserKey(std::string_view inner_key) -> std::string_view {
  return inner_key.substr(0, inner_key.size() - 9);  // 将
}

/**
 * @brief  获取inner_key中的seq
 *
 * @param inner_key
 * @return int64_t seq
 */
auto InnerKeySeq(std::string_view inner_key) -> int64_t {
  int64_t seq;
  memcpy(&seq, &inner_key[inner_key.length() - 9], 8);
  return seq;
}
/**
 * @brief 获取inner_key中的操作类型
 *
 * @param inner_key
 * @return OperatorType 操作类型
 */
auto InnerKeyOpType(std::string_view inner_key) -> OperatorType {
  return static_cast<OperatorType>(*const_cast<char *>(inner_key.substr(inner_key.size() - 1, 1).data()));
}

/**
 * @brief 比较两个inner_key的user_key
 *
 * @param k1
 * @param k2
 * @return int 0: k1 == k2, 1: k1 > k2, -1: k1 < k2
 */
auto CmpUserKeyOfInnerKey(std::string_view k1, std::string_view k2) -> int {
  return InnerKeyToUserKey(k1).compare(InnerKeyToUserKey(k2));
}

/**
 * @brief 比较两个inner_key
 * 首先比较user_key, 如果相等, 则比较seq, 如果seq相等, 则比较op
 *
 * @param k1
 * @param k2
 * @return int 0: k1 == k2, 1: k1 > k2, -1: k1 < k2
 */
auto CmpInnerKey(std::string_view k1, std::string_view k2) -> int {
  if (int ret = CmpUserKeyOfInnerKey(k1, k2); ret) {
    return ret;
  }
  /* seq op 反过来比较 */
  int64_t seq1 = InnerKeySeq(k1);
  int64_t seq2 = InnerKeySeq(k2);
  if (seq1 == seq2) {
    OperatorType op1 = InnerKeyOpType(k1);
    OperatorType op2 = InnerKeyOpType(k2);
    return static_cast<int>(op2) - static_cast<int>(op1);
  }
  return seq2 - seq1 > 0 ? 1 : -1;
}

auto CmpKeyAndUserKey(std::string_view key, std::string_view user_key) -> int {
  std::string_view key_user = InnerKeyToUserKey(key);
  return key_user.compare(user_key);
}

/**
 * @brief  保存结果到dk, dv中
 *
 * @param rk 从sstable中读取的key
 * @param rv 从sstable中读取的value
 * @param tk 用户查询的key
 * @param dk 保存的key
 * @param dv 保存的value
 * @return RC OK: 保存成功; NOT_FOUND: 保存失败
 */
auto SaveResultIfUserKeyMatch(std::string_view rk, std::string_view rv, std::string_view tk, std::string &dk,
                              std::string &dv) -> RC {
  if (CmpUserKeyOfInnerKey(rk, tk) != 0) {
    return RC::NOT_FOUND;
  }
  if (InnerKeyOpType(rk) == OperatorType::DELETE) {
    return RC::NOT_FOUND;
  }
  dk.assign(rk.data(), rk.length());
  dv.assign(rv.data(), rv.length());
  return RC::OK;
}

auto NewMinInnerKey(std::string_view key) -> std::string { return MemKey::NewMinMemKey(key).ToSSTableKey(); }

auto EasyCmp(std::string_view key1, std::string_view key2) -> int { return CmpInnerKey(key1, key2); }

auto EasySaveValue(std::string_view rk, std::string_view rv, std::string_view tk, std::string &dk, std::string &dv)
    -> RC {
  return SaveResultIfUserKeyMatch(rk, rv, tk, dk, dv);
}

auto EncodeKVPair(const MemKey &key, std::string_view value) -> std::string {
  return key.ToSSTableKey() + std::string(value);
}

void DecodeKVPair(std::string_view data, MemKey &memkey, std::string &value) {}

}  // namespace lsm_tree