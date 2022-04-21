#pragma once

#include <stdexcept>

#include "fast_tree/string_formatter.h"

namespace fast_tree {

template <typename T>
class span {
 public:
  typedef T value_type;

  span(const value_type* data, size_t size) :
      data_(data),
      size_(size)
  {
  }

  template <size_t S>
  span(const value_type (&data)[S]) :
      data_(data),
      size_(S)
  {
  }

  span(const span& ref) = default;

  span& operator=(const span& rhs) = default;

  const value_type* data() const {
    return data_;
  }

  size_t size() const {
    return size_;
  }

  const value_type& operator[](size_t i) const {
    return data_[i];
  }

  const value_type& at(size_t i) const {
    if (i >= size_) {
      throw std::out_of_range(string_formatter()
                              << "Index " << i << " out of range (max " << size_ << ")");
    }
    return data_[i];
  }

 private:
  const value_type* data_ = nullptr;
  size_t size_ = 0;
};

}
