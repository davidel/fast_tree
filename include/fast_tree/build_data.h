#pragma once

#include <algorithm>
#include <type_traits>
#include <vector>

#include "fast_tree/data.h"
#include "fast_tree/span.h"
#include "fast_tree/storage_span.h"
#include "fast_tree/types.h"
#include "fast_tree/util.h"

namespace fast_tree {

template <typename T>
class build_data {
 public:
  using value_type = T;

  explicit build_data(const fast_tree::data<T>& xdata) :
      data_(xdata),
      indices_(iota(data_.num_rows())),
      start_(0),
      end_(data_.num_rows()) {
  }

  build_data(const fast_tree::data<T>& xdata, std::vector<size_t> indices) :
      data_(xdata),
      indices_(std::move(indices)),
      start_(0),
      end_(indices_.size()) {
  }

  build_data(const build_data& parent, size_t istart, size_t end) :
      data_(parent.data()),
      indices_(parent.indices_),
      start_(istart),
      end_(end) {
  }

  span<size_t> indices() const {
    return indices_.data().subspan(start_, end_ - start_);
  }

  const fast_tree::data<T>& data() const {
    return data_;
  }

  size_t start() const {
    return start_;
  }

  size_t end() const {
    return end_;
  }

  std::vector<std::remove_cv_t<T>> target() const {
    return take(data_.target().data(), indices());
  }

  std::vector<std::remove_cv_t<T>> column(size_t i) const {
    return data_.column_sample(i, indices());
  }

  template <typename U>
  span<U> column(size_t i, span<U> out) const {
    return data_.column_sample(i, indices(), out);
  }

  size_t split_indices(size_t i, T split_value) {
    typename fast_tree::data<T>::cdata col = data_.column(i);
    span<size_t> idx = indices();
    size_t pos = 0;
    size_t end = idx.size();

    while (pos + 1 < end) {
      size_t x = idx[pos];

      if (col[x] < split_value) {
        ++pos;
      } else {
        std::swap(idx[pos], idx[end - 1]);
        --end;
      }
    }

    return start_ + pos;
  }

 private:
  const fast_tree::data<T>& data_;
  storage_span<size_t> indices_;
  size_t start_;
  size_t end_;
};

}
