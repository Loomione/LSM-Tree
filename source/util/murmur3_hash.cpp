#include "util/murmur3_hash.hh"

namespace lsm_tree {

static auto RotateLeft(int value, int32_t count) -> int {
  int mask = 8 * sizeof(int) - 1;
  count &= mask;
  return ((value << count) | (value >> ((-count) & mask)));
}

auto Murmur3Hash(uint32_t seed, const char *data, size_t len) -> uint32_t {
  const uint32_t c1 = 0xcc9e2d51;
  const uint32_t c2 = 0x1b873593;
  const uint32_t r1 = 15;
  const uint32_t r2 = 13;
  const uint32_t m  = 5;
  const uint32_t n  = 0xe6546b64;
  int            i;
  uint32_t       k1 = 0;
  const char    *tail;

  int len4 = len / sizeof(uint32_t);

  uint32_t k;
  for (i = 0; i < len4; i++) {
    auto byte1 = static_cast<uint32_t>(data[4 * i]);
    auto byte2 = (static_cast<uint32_t>(data[4 * i + 1])) << 8;
    auto byte3 = (static_cast<uint32_t>(data[4 * i + 2])) << 16;
    auto byte4 = (static_cast<uint32_t>(data[4 * i + 3])) << 24;
    k          = byte1 | byte2 | byte3 | byte4;
    k *= c1;
    k = RotateLeft(k, r1);
    k *= c2;

    seed ^= k;
    seed = RotateLeft(seed, r2) * m + n;
  }

  tail = (data + len4 * sizeof(uint32_t));

  switch (len & (sizeof(uint32_t) - 1)) {
    case 3:
      k1 ^= (static_cast<uint32_t>(tail[2])) << 16;
      /*-fallthrough*/
    case 2:
      k1 ^= (static_cast<uint32_t>(tail[1])) << 8;
      /*-fallthrough*/
    case 1:
      k1 ^= (static_cast<uint32_t>(tail[0])) << 0;
      k1 *= c1;
      k1 = RotateLeft(k1, r1);
      k1 *= c2;
      seed ^= k1;
      break;
  }

  seed ^= static_cast<uint32_t>(len);
  seed ^= (seed >> 16);
  seed *= 0x85ebca6b;
  seed ^= (seed >> 13);
  seed *= 0xc2b2ae35;
  seed ^= (seed >> 16);

  return seed;
}

}  // namespace lsm_tree