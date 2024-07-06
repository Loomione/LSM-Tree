#pragma once

#include <openssl/sha.h>
#include <optional>
#include <string>

namespace lsm_tree {

using std::string;
using std::string_view;

auto HexStringToInt(const std::string_view &input) -> std::optional<int>;
auto Sha256DigitToHex(const unsigned char hash[SHA256_DIGEST_LENGTH]) -> string;
void HexToSha256Digit(string_view hex, unsigned char *hash);
}  // namespace lsm_tree
