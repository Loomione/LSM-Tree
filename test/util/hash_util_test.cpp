#include "util/hash_util.hh"
#include "gtest/gtest.h"

TEST(HashUtil, HexStringToInt) {
  {
    std::string hex    = "1234";
    auto        result = lsm_tree::HexStringToInt(hex);
    EXPECT_EQ(result.value(), 0x1234);
  }

  {
    std::string hex    = "fffff";
    auto        result = lsm_tree::HexStringToInt(hex);
    EXPECT_EQ(result.value(), 0xfffff);
  }
}

TEST(HashUtil, Sha256DigitToHex) {
  std::string hash_str = "123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0";
  unsigned char hash[SHA256_DIGEST_LENGTH];
  lsm_tree::HexToSha256Digit(hash_str, hash);
  std::string hex = lsm_tree::Sha256DigitToHex(hash);
  EXPECT_EQ(hex, hash_str);
}