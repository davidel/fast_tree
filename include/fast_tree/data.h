#pragma once

#include <memory>
#include <random>
#include <stdexcept>
#include <vector>

#include "fast_tree/span.h"
#include "fast_tree/string_formatter.h"
#include "fast_tree/types.h"
#include "fast_tree/util.h"

namespace fast_tree {

template <typename T>
class column_data {
 public:
  column_data(span<const T> data) :
      data_(data) {
  }

  column_data(std::vector<T>&& storage) :
      storage_(std::move(storage)),
      data_(storage_) {
  }

  column_data(const column_data& ref) :
      data_(ref.data()) {
  }

  explicit column_data(column_data&& ref) = default;

  column_data& operator=(const column_data& ref) {
    data_ = ref.data();

    return *this;
  }

  size_t size() const {
    return data_.size();
  }

  const T& operator[](size_t i) const {
    return data_[i];
  }

  span<const T> data() const {
    return data_;
  }

 private:
  std::vector<T> storage_;
  span<const T> data_;
};

template <typename T>
class data {
 public:
  using value_type = T;

  using cdata = column_data<T>;

  virtual ~data() = default;

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
    if (!columns_.empty() && columns_[0].size() != col.size()) {
      throw std::invalid_argument(string_formatter()
                                  << "All columns must have the same size: "
                                  << columns_[0].size() << " != " << col.size());
    }
    columns_.push_back(std::move(col));
  }

 private:
  std::vector<cdata> columns_;
};

template <typename T>
template <typename G>
std::unique_ptr<data<T>> data<T>::resample(size_t nrows, G* rgen) const {
  std::vector<size_t> row_indices = fast_tree::resample(num_rows(), nrows, rgen);

  return std::make_unique<sampled_data<T>>(*this, std::move(row_indices));
}

}
