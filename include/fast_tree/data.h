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
    return columns_.empty() ? 0: columns_[0].size();
  }

  cdata column(size_t i) const {
    return columns_.at(i).data();
  }

  std::vector<std::remove_cv_t<T>> column_sample(size_t i, span<const size_t> indices) const {
    return take(columns_.at(i).data(), indices);
  }

  template <typename U>
  span<U> column_sample(size_t i, span<const size_t> indices, span<U> out) const {
    return take(columns_.at(i).data(), indices, out);
  }

  size_t add_column(cdata col) {
    FT_ASSERT(target_.size() == col.size())
        << "All columns must have the same size: "
        << target_.size() << " != " << col.size();

    columns_.push_back(std::move(col));

    return columns_.size() - 1;
  }

 private:
  std::vector<cdata> columns_;
  cdata target_;
};

}
