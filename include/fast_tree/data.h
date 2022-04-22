#pragma once

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

  explicit column_data(std::vector<T> storage) :
      storage_(std::move(storage)),
      data_(storage_) {
  }

  explicit column_data(column_data&& ref) = default;

  column_data(const column_data& ref) = delete;

  column_data& operator=(const column_data& ref) = delete;

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
  typedef T value_type;

  typedef column_data<T> cdata;

  virtual size_t num_columns() const = 0;

  virtual size_t num_rows() const = 0;

  virtual cdata column(size_t i) const = 0;
};

template <typename T>
class real_data : public data<T> {
 public:
  using cdata = typename data<T>::cdata;

  virtual size_t num_columns() const override {
    return columns_.size();
  }

  virtual size_t num_rows() const override {
    return columns_.empty() ? 0: columns_[0].data().size();
  }

  virtual cdata column(size_t i) const override {
    return columns_.at(i).data();
  }

  void add_column(cdata col) {
    if (!columns_.empty() && columns_[0].data().size() != col.data().size()) {
      throw std::invalid_argument(string_formatter()
                                  << "All columns must have the same size: "
                                  << columns_[0].data().size() << " != "
                                  << col.data().size());
    }
    columns_.push_back(std::move(col));
  }

 private:
  std::vector<cdata> columns_;
};

}
