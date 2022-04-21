#pragma once

#include <stdexcept>

namespace fast_tree {

template <typename T>
class span {
 public:
  typedef T value_type;

  span(const value_type* ptr, size_t size) :
      ptr_(ptr),
      size_(size)
  {
  }

  template <size_t S>
  span(const value_type data[S]) :
      ptr_(data),
      size_(S)
  {
  }

  span(const span& ref) = default;

  span& operator=(const span& rhs) = default;

  const value_type& operator[](size_t i) const {
    return ptr_[i];
  }

 private:
  const value_type* ptr_ = nullptr;
  size_t size_ = 0;
};

}
