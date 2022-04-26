#pragma once

#include <memory>
#include <random>
#include <stdexcept>
#include <vector>

#include "fast_tree/span.h"
#include "fast_tree/storage_span.h"
#include "fast_tree/string_formatter.h"
#include "fast_tree/types.h"
#include "fast_tree/util.h"

namespace fast_tree {

template <typename T>
class data {
 public:
  using value_type = T;

  using cdata = storage_span<T>;

  virtual ~data() = default;

  virtual cdata target() const = 0;

  virtual size_t num_columns() const = 0;

  virtual size_t num_rows() const = 0;

  virtual cdata column(size_t i) const = 0;

  virtual std::vector<T> column_sample(size_t i, span<const size_t> indices) const = 0;

  template <typename G>
  std::unique_ptr<data> resample(size_t nrows, G* rgen) const;
};

template <typename T>
class sampled_data : public data<T> {
 public:
  using cdata = typename data<T>::cdata;

  sampled_data(const data<T>& ref_data, std::vector<size_t> row_indices) :
      ref_data_(ref_data),
      row_indices_(std::move(row_indices)) {
  }

  sampled_data(sampled_data&& ref) = default;

  virtual cdata target() const override {
    cdata tgt = ref_data_.target();

    return cdata(take(tgt.data(), row_indices_));
  }

  virtual size_t num_columns() const override {
    return ref_data_.num_columns();
  }

  virtual size_t num_rows() const override {
    return row_indices_.size();
  }

  virtual cdata column(size_t i) const override {
    cdata rcol = ref_data_.column(i);

    return cdata(take(rcol.data(), row_indices_));
  }

  virtual std::vector<T> column_sample(
      size_t i, span<const size_t> indices) const override {
    cdata rcol = ref_data_.column(i);
    std::vector<T> col;

    col.reserve(indices.size());
    for (size_t ix : indices) {
      col.push_back(rcol[row_indices_[ix]]);
    }

    return col;
  }

 private:
  const data<T>& ref_data_;
  std::vector<size_t> row_indices_;
};

template <typename T>
class real_data : public data<T> {
 public:
  using cdata = typename data<T>::cdata;

  explicit real_data(cdata target) :
      target_(std::move(target)) {
  }

  virtual cdata target() const override {
    return target_;
  }

  virtual size_t num_columns() const override {
    return columns_.size();
  }

  virtual size_t num_rows() const override {
    return columns_.empty() ? 0: columns_[0].size();
  }

  virtual cdata column(size_t i) const override {
    return columns_.at(i).data();
  }

  virtual std::vector<T> column_sample(size_t i, span<const size_t> indices) const override {
    return take(columns_.at(i).data(), indices);
  }

  void add_column(cdata col) {
    if (target_.size() != col.size()) {
      throw std::invalid_argument(string_formatter()
                                  << "All columns must have the same size: "
                                  << target_.size() << " != " << col.size());
    }
    columns_.push_back(std::move(col));
  }

 private:
  std::vector<cdata> columns_;
  cdata target_;
};

template <typename T>
template <typename G>
std::unique_ptr<data<T>> data<T>::resample(size_t nrows, G* rgen) const {
  std::vector<size_t> row_indices = fast_tree::resample(num_rows(), nrows, rgen);

  return std::make_unique<sampled_data<T>>(*this, std::move(row_indices));
}

}
