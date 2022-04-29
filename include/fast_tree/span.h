#pragma once

#include <array>
#include <stdexcept>
#include <vector>

#include "fast_tree/string_formatter.h"
#include "fast_tree/types.h"

namespace fast_tree {

template <typename T>
class span {
 public:
  using value_type = T;

  using iterator = value_type*;

  static constexpr size_t no_size = static_cast<size_t>(-1);

  span() = default;

  span(value_type* data, size_t size) :
      data_(data),
      size_(size) {
  }

  template <size_t N>
  span(value_type (&data)[N]) :
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
  span(fvector<U>& data) :
      data_(data.data()),
      size_(data.size()) {
  }

  template <typename U>
  span(const fvector<U>& data) :
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
    return data();
  }

  iterator end() const {
    return data() + size();
  }

  value_type* data() const {
    return data_;
  }

  size_t size() const {
    return size_;
  }

  bool empty() const {
    return size() == 0;
  }

  value_type& front() const {
    if (empty()) {
      throw std::out_of_range("Tring to access empty span");
    }
    return data()[0];
  }

  value_type& back() const {
    if (empty()) {
      throw std::out_of_range("Tring to access empty span");
    }
    return data()[size() - 1];
  }

  value_type& operator[](size_t i) const {
    return data_[i];
  }

  value_type& at(size_t i) const {
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
  value_type* data_ = nullptr;
  size_t size_ = 0;
};

}
