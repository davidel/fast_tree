#pragma once

#include <algorithm>
#include <cstddef>
#include <span>
#include <type_traits>
#include <vector>

#include "dcpl/assert.h"
#include "dcpl/storage_span.h"
#include "dcpl/types.h"
#include "dcpl/utils.h"

#include "fast_tree/data.h"

namespace fast_tree {

template <typename T>
class build_data {
 public:
  using value_type = T;
  using rvalue_type = typename data<T>::rvalue_type;

  explicit build_data(const data<T>& xdata) :
      data_(xdata),
      indices_(dcpl::iota<std::size_t>(data_.num_rows())),
      start_(0),
      end_(indices_.size()) {
  }

  build_data(const data<T>& xdata, std::vector<std::size_t> indices) :
      data_(xdata),
      indices_(std::move(indices)),
      start_(0),
      end_(indices_.size()) {
  }

  build_data(const build_data& parent, std::size_t start, std::size_t end) :
      data_(parent.data()),
      indices_(parent.indices_),
      start_(start),
      end_(end) {
  }

  std::size_t size() const {
    return end_ - start_;
  }

  std::span<std::size_t> indices() const {
    return indices_.data().subspan(start(), size());
  }

  const data<T>& data() const {
    return data_;
  }

  std::size_t start() const {
    return start_;
  }

  std::size_t end() const {
    return end_;
  }

  std::vector<rvalue_type> target() const {
    return dcpl::take(data_.target().data(), indices());
  }

  template <typename U>
  std::span<U> target(std::span<U> out) const {
    return dcpl::take(data_.target().data(), indices(), out);
  }

  std::vector<rvalue_type> column(std::size_t i) const {
    return data_.column_sample(i, indices());
  }

  template <typename U>
  std::span<U> column(std::size_t i, std::span<U> out) const {
    return data_.column_sample(i, indices(), out);
  }

  std::size_t partition_indices(std::size_t i, T pivot) {
    typename fast_tree::data<T>::cdata col = data_.column(i);
    std::span<std::size_t> idx = indices();
    std::size_t pos = 0;
    std::size_t top = idx.size();

    while (pos < top) {
      std::size_t x = idx[pos];

      if (col[x] < pivot) {
        ++pos;
      } else {
        std::swap(idx[pos], idx[top - 1]);
        --top;
      }
    }

    return start_ + pos;
  }

 private:
  const fast_tree::data<T>& data_;
  dcpl::storage_span<std::size_t> indices_;
  std::size_t start_ = 0;
  std::size_t end_ = 0;
};

}
