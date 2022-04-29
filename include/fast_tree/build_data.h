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
      indices_(iota(data_.num_rows())),
      mask_(data_.num_rows(), true),
      sorted_col_indices_(data_.num_columns()) {
  }

  build_data(std::shared_ptr<build_data> parent, fvector<size_t> sub_indices) :
      parent_(std::move(parent)),
      data_(parent_->data()),
      indices_(std::move(sub_indices)),
      mask_(create_bitmap(data_.num_rows(), indices_)),
      sorted_col_indices_(data_.num_columns()) {
  }

  span<const size_t> indices() const {
    return indices_;
  }

  const fast_tree::data<T>& data() const {
    return data_;
  }

  fvector<T> target() const {
    return take(data_.target().data(), indices_);
  }

  fvector<T> column(size_t i) {
    return data_.column_sample(i, column_indices(i));
  }

  span<const size_t> column_indices(size_t i) {
    if (sorted_col_indices_[i].empty()) {
      if (parent_ == nullptr) {
        typename fast_tree::data<T>::cdata col = data_.column(i);

        sorted_col_indices_[i] = argsort(col);
      } else {
        sorted_col_indices_[i] = reduce_indices(parent_->column_indices(i), mask_);
      }
    }

    return sorted_col_indices_[i];
  }

  fvector<span<const size_t>> split_indices(size_t colno, size_t split_index) {
    span<const size_t> col_indices = column_indices(colno);
    fvector<span<const size_t>> splits;

    if (split_index > 0) {
      splits.push_back(col_indices.subspan(0, split_index));
    }
    if (split_index < col_indices.size()) {
      splits.push_back(col_indices.subspan(split_index));
    }

    return splits;
  }

 private:
  std::shared_ptr<build_data> parent_;
  const fast_tree::data<T>& data_;
  fvector<size_t> indices_;
  bitmap mask_;
  fvector<fvector<size_t>> sorted_col_indices_;
};

}
