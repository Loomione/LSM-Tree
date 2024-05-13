#pragma once

#include <functional>
#include <utility>

namespace lsm_tree {

class Defer {
  std::function<void()> f_;

 public:
  explicit Defer(std::function<void()> f) : f_(std::move(f)) {}
  ~Defer() { f_(); }
};

}  // namespace lsm_tree