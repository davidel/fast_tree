#pragma once

#include <memory>
#include <numeric>
#include <vector>

#include "fast_tree/data.h"
#include "fast_tree/span.h"
#include "fast_tree/types.h"
#include "fast_tree/util.h"

namespace fast_tree {

template <typename T>
class build_data {
 public:
  using value_type = T;

  explicit build_data(const fast_tree::data<T>& xdata) :
      data_(xdata),
      indices_(iota(xdata.num_rows())),
      sorted_col_indices_(create_sorted_indices(xdata)) {
  }

  build_data(const build_data& parent, std::vector<size_t> indices) :
      data_(parent.data()),
      indices_(std::move(indices)),
      sorted_col_indices_(create_sorted_indices(parent.sorted_col_indices_,
                                                indices_, data_.num_rows())) {
  }

  const fast_tree::data<T>& data() const {
    return data_;
  }

  std::vector<T> column(size_t i) const {
    return data_.column_sample(i, sorted_col_indices_[i]);
  }

  span<const size_t> column_indices(size_t i) const {
    return sorted_col_indices_[i];
  }

 private:
  static std::vector<std::vector<size_t>> create_sorted_indices(
      const fast_tree::data<T>& xdata) {
    std::vector<std::vector<size_t>> colidx;

    colidx.reserve(xdata.num_columns());
    for (size_t i = 0; i < xdata.num_columns(); ++i) {
      typename fast_tree::data<T>::cdata col = xdata.column(i);

      colidx.push_back(argsort(col));
    }

    return colidx;
  }

  static std::vector<std::vector<size_t>> create_sorted_indices(
      const std::vector<std::vector<size_t>>& sorted_col_indices,
      span<const size_t> sub_indices, size_t size) {
    std::vector<std::vector<size_t>> colidx;

    colidx.reserve(sorted_col_indices.size());

    bitmap mask = create_bitmap(size, sub_indices);

    for (size_t i = 0; i < sorted_col_indices.size(); ++i) {
      colidx.push_back(reduce_indices(sorted_col_indices[i], mask));
    }

    return colidx;
  }

  const fast_tree::data<T>& data_;
  std::vector<size_t> indices_;
  std::vector<std::vector<size_t>> sorted_col_indices_;
};

}
