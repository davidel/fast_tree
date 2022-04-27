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

}
