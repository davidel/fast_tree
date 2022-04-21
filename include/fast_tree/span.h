#pragma once

#include <stdexcept>
#include <type_traits>
#include <vector>

#include "fast_tree/string_formatter.h"

namespace fast_tree {

template <typename T>
class span {
 public:
  typedef std::remove_cv_t<T> value_type;

  typedef const value_type* iterator;

  static constexpr size_t no_size = static_cast<size_t>(-1);

  span(const value_type* data, size_t size) :
      data_(data),
      size_(size) {
  }

  template <size_t S>
  span(const value_type (&data)[S]) :
      data_(data),
      size_(S) {
  }

  span(const std::vector<value_type>& ref) :
      data_(ref.data()),
      size_(ref.size()) {
  }

  span(const span& ref) = default;

  span& operator=(const span& rhs) = default;

  iterator begin() const {
    return data();
  }

  iterator end() const {
    return data() + size();
  }

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

  span subspan(size_t pos, size_t size = no_size) const {
    if (pos > size_) {
      throw std::out_of_range(string_formatter()
                              << "Position " << pos << " out of range (max " << size_ << ")");
    }
    if (size == no_size) {
      size = size_ - pos;
    }
    if (pos + size > size_) {
      throw std::out_of_range(string_formatter()
                              << "Position " << (pos + size) << " out of range (max " << size_ << ")");
    }

    return span(data_ + pos, size);
  }

 private:
  const value_type* data_ = nullptr;
  size_t size_ = 0;
};

}
