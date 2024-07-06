#include "util/hash_util.hh"
#include <charconv>
#include <iomanip>

namespace lsm_tree {

/**
 * @brief  将16进制字符串转换为int
 *
 * @param input
 * @return std::optional<int>
 */
auto HexStringToInt(const std::string_view &input) -> std::optional<int> {
  unsigned int                 out;
  const std::from_chars_result result = std::from_chars(input.data(), input.data() + input.size(), out, 16);
  if (result.ec == std::errc::invalid_argument || result.ec == std::errc::result_out_of_range) {
    return std::nullopt;
  }
  return out;
}

/**
 * @brief  将hash转换为16进制字符串
 *
 * @param hash
 * @return string
 */
auto Sha256DigitToHex(const unsigned char hash[]) -> string {
  std::stringstream ss;
  ss.setf(std::ios::hex, std::ios::basefield);

  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    ss.width(2);
    ss.fill('0');
    ss << static_cast<unsigned int>(hash[i]);
  }
  ss.setf(std::ios::dec, std::ios::basefield);
  return ss.str();
}

/**
 * @brief  将16进制字符串转换为hash
 *
 * @param hex
 * @param hash
 */
void HexToSha256Digit(string_view hex, unsigned char *hash) {
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    hash[i] = static_cast<unsigned char>(HexStringToInt(hex.substr(i * 2, 2)).value());
  }
}
}  // namespace lsm_tree
