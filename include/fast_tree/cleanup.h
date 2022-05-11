#pragma once

#include <functional>

namespace fast_tree {

class cleanup {
 public:
  explicit cleanup(std::function<void ()> fn) :
      fn_(std::move(fn)) {
  }

  ~cleanup() {
    if (fn_ != nullptr) {
      fn_();
    }
  }

  void reset() {
    fn_ = nullptr;
  }

 private:
  std::function<void ()> fn_;
};

}
