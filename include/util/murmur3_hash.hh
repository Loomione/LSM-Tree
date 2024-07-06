#pragma once
#include <cstddef>
#include <cstdint>
namespace lsm_tree {

//  https://en.wikipedia.org/wiki/MurmurHash#Algorithm

auto Murmur3Hash(uint32_t seed, const char *data, size_t len) -> uint32_t;

}  // namespace lsm_tree
