#pragma once

#include <cstddef>
#include <memory>
#include <random>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "dcpl/assert.h"
#include "dcpl/storage_span.h"
#include "dcpl/types.h"
#include "dcpl/utils.h"

namespace fast_tree {

template <typename T>
class data {
 public:
  using value_type = T;
  using rvalue_type = std::remove_cv_t<value_type>;
  using cdata = dcpl::storage_span<T>;

  explicit data(cdata target) :
      target_(std::move(target)) {
  }

  cdata target() const {
    return target_;
  }

  std::size_t num_columns() const {
    return columns_.size();
  }

  std::size_t num_rows() const {
    return target_.size();
  }

  template <typename U>
  std::span<U> row(std::size_t i, std::span<U> out) const {
    DCPL_ASSERT(i < num_rows())
        << "Row " << i << " is out of range (max " << num_rows() << ")";
    DCPL_ASSERT(out.size() >= num_columns())
        << "Buffer size too small: " << out.size() << " vs. " << num_columns();

    U* data = out.data();

    for (std::size_t c = 0; c < num_columns(); ++c) {
      *data++ = columns_[c][i];
    }

    return std::span<U>(out.data(), data - out.data());
  }

  std::vector<rvalue_type> row(std::size_t i) const {
    std::vector<rvalue_type> row_values(num_columns());

    row(i, std::span<rvalue_type>(row_values));

    return row_values;
  }

  cdata column(std::size_t i) const {
    return columns_.at(i);
  }

  std::vector<rvalue_type> column_sample(
      std::size_t i, std::span<const std::size_t> indices) const {
    return dcpl::take(columns_.at(i).data(), indices);
  }

  template <typename U>
  std::span<U> column_sample(std::size_t i, std::span<const std::size_t> indices,
                             std::span<U> out) const {
    return dcpl::take(columns_.at(i).data(), indices, out);
  }

  std::size_t add_column(cdata col) {
    DCPL_ASSERT(target_.size() == col.size())
        << "Columns must have the same size of the target: "
        << target_.size() << " != " << col.size();

    columns_.push_back(std::move(col));

    return columns_.size() - 1;
  }

 private:
  cdata target_;
  std::vector<cdata> columns_;
};

}
