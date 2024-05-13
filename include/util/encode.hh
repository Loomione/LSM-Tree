#pragma once

#include <string>
#include <string_view>

namespace lsm_tree {

using std::string;
using std::string_view;
void Decode32(const char *src, int *dest);
void Encode32(int src, char *dest);
void EncodeWithPreLen(string &dest, string_view data);
auto DecodeWithPreLen(string &dest, string_view data) -> int;
void Decode64(const char *src, int64_t *dest);

}  // namespace lsm_tree
