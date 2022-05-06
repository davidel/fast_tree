#pragma once

#include <iostream>
#include <array>
#include <vector>

#include "fast_tree/assert.h"

namespace fast_tree {

// Can't use -std=c++-20 ATM.
template <typename T>
class span {
 public:
  using value_type = T;

  using iterator = T*;

  static constexpr size_t no_size = static_cast<size_t>(-1);

  span() = default;

  span(T* data, size_t size) :
      data_(data),
      size_(size) {
  }

  template <size_t N>
  span(T (&data)[N]) :
      data_(data),
      size_(N) {
  }

  template <typename U, size_t N>
  span(std::array<U, N>& data) :
      data_(data.data()),
      size_(N) {
  }

  template <typename U, size_t N>
  span(const std::array<U, N>& data) :
      data_(data.data()),
      size_(N) {
  }

  template <typename U>
  span(std::vector<U>& data) :
      data_(data.data()),
      size_(data.size()) {
  }

  template <typename U>
  span(const std::vector<U>& data) :
      data_(data.data()),
      size_(data.size()) {
  }

  template <typename U>
  span(const span<U>& ref) :
      data_(ref.data()),
      size_(ref.size()) {
  }

  span& operator=(const span& rhs) = default;

  iterator begin() const {
    return data_;
  }

  iterator end() const {
    return data_ + size_;
  }

  T* data() const {
    return data_;
  }

  size_t size() const {
    return size_;
  }

  bool empty() const {
    return size_ == 0;
  }

  T& front() const {
    FT_ASSERT(!empty()) << "Tring to access empty span";

    return data_[0];
  }

  T& back() const {
    FT_ASSERT(!empty()) << "Tring to access empty span";

    return data_[size_ - 1];
  }

  bool operator==(const span& rhs) const {
    if (size_ != rhs.size_) {
      return false;
    }

    for (size_t i = 0; i < size_; ++i) {
      if (data_[i] != rhs.data_[i]) {
        return false;
      }
    }

    return true;
  }

  bool operator!=(const span& rhs) const {
    return !operator==(rhs);
  }

  T& operator[](size_t i) const {
    return data_[i];
  }

  T& at(size_t i) const {
    FT_ASSERT(i < size_) << "Index " << i << " out of range (max " << size_ << ")";

    return data_[i];
  }

  span subspan(size_t pos, size_t size = no_size) const {
    FT_ASSERT(pos <= size_) << "Position " << pos << " out of range (max " << size_ << ")";

    if (size == no_size) {
      size = size_ - pos;
    } else {
      FT_ASSERT(pos + size <= size_) << "Position " << (pos + size) << " out of range (max "
                                     << size_ << ")";
    }

    return span(data_ + pos, size);
  }

  friend std::ostream& operator<<(std::ostream& os, const span& sp) {
    for (size_t i = 0; i < sp.size(); ++i) {
      if (i > 0) {
        os << ", ";
      }
      os << sp[i];
    }

    return os;
  }

 private:
  T* data_ = nullptr;
  size_t size_ = 0;
};

}
