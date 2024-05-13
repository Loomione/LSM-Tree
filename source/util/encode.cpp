#include "util/encode.hh"
#include <cstring>
#include <string>

namespace lsm_tree {

/**
 * @brief 将src中的前4个字节解析为int
 *
 * @param src
 * @param dest
 */
void Decode32(const char *src, int *dest) { *dest = *reinterpret_cast<const int *>(src); }

/**
 * @brief 将src编码为4字节的char数组
 *
 * @param src
 * @param dest
 */
void Encode32(int src, char *dest) { memcpy(dest, &src, sizeof(int)); }

/**
 * @brief 将data编码为带有长度前缀的字符串
 *
 * @param dest
 * @param data
 */
void EncodeWithPreLen(string &dest, string_view data) {
  int len = data.size();
  dest.append(reinterpret_cast<const char *>(&len), sizeof(int));
  dest.append(data.data(), len);
}

/**
 * @brief 解析带有长度前缀的字符串
 *
 * @param dest
 * @param data
 * @return int
 */
auto DecodeWithPreLen(string &dest, string_view data) -> int {
  int         len;
  const char *buf = data.data();
  Decode32(buf, &len);
  dest.assign(buf + sizeof(int), len);
  return len + sizeof(int);
}

/**
 * @brief 将src中的前8个字节解析为int64_t
 *
 * @param src
 * @param dest
 */
void Decode64(const char *src, int64_t *dest) { *dest = *(int64_t *)(src); }

}  // namespace lsm_tree