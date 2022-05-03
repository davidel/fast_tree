#pragma once

#include <memory>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "fast_tree/assert.h"
#include "fast_tree/span.h"
#include "fast_tree/storage_span.h"
#include "fast_tree/types.h"
#include "fast_tree/util.h"

namespace fast_tree {

template <typename T>
class data {
 public:
  using value_type = T;

  using cdata = storage_span<T>;

  explicit data(cdata target) :
      target_(std::move(target)) {
  }

  cdata target() const {
    return target_;
  }

  size_t num_columns() const {
    return columns_.size();
  }

  size_t num_rows() const {
    return target_.size();
  }

  std::vector<T> row(size_t i) const {
    FT_ASSERT(i < num_rows())
        << "Row " << i << " is out of range (max " << num_rows() << ")";

    std::vector<T> row_values;

    row_values.reserve(num_columns());
    for (size_t c = 0; c < num_columns(); ++c) {
      row_values.push_back(columns_[c][i]);
    }

    return row_values;
  }

  template <typename U>
  span<U> row(size_t i, span<U> out) const {
    FT_ASSERT(i < num_rows())
        << "Row " << i << " is out of range (max " << num_rows() << ")";
    FT_ASSERT(out.size() >= num_columns())
        << "Buffer size too small: " << out.size() << " vs. " << num_columns();

    U* data = out.data();
    size_t count = 0;

    for (size_t c = 0; c < num_columns(); ++c) {
      data[count++] = columns_[c][i];
    }

    return span<U>(data, count);
  }

  cdata column(size_t i) const {
    return columns_.at(i);
  }

  std::vector<std::remove_cv_t<T>> column_sample(
      size_t i, span<const size_t> indices) const {
    return take(columns_.at(i).data(), indices);
  }

  template <typename U>
  span<U> column_sample(size_t i, span<const size_t> indices, span<U> out) const {
    return take(columns_.at(i).data(), indices, out);
  }

  size_t add_column(cdata col) {
    FT_ASSERT(target_.size() == col.size())
        << "Columns must have the same size of the target: "
        << target_.size() << " != " << col.size();

    columns_.push_back(std::move(col));

    return columns_.size() - 1;
  }

 private:
  std::vector<cdata> columns_;
  cdata target_;
};

}
