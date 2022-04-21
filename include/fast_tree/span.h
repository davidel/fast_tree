#pragma once

template <typename T>
class Span {
 public:
  typedef T value_type;

  Span(const value_type* ptr, size_t size) :
      ptr_(ptr),
      size_(size)
  {
  }

  template <size_t S>
  Span(const value_type data[S]) :
      ptr_(data),
      size_(S)
  {
  }

  Span(const Span& ref) = default;

  Span& operator=(const Span& rhs) = default;

  const value_type& operator[](size_t i) const {
    return ptr_[i];
  }

 private:
  const value_type* ptr_ = nullptr;
  size_t size_ = 0;
};
