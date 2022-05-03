#pragma once

#include <memory>
#include <type_traits>
#include <vector>

#include "fast_tree/span.h"
#include "fast_tree/types.h"

namespace fast_tree {

template <typename T>
class storage_span {
 public:
  using value_type = T;

  using vector_type = std::vector<std::remove_cv_t<T>>;

  storage_span() = default;

  storage_span(span<T> data) :
      data_(data) {
  }

  storage_span(vector_type&& stg) :
      storage_(std::make_shared<vector_type>(std::move(stg))),
      data_(*storage_) {
  }

  storage_span(const storage_span&) = default;

  storage_span(storage_span&&) = default;

  storage_span& operator=(const storage_span&) = default;

  storage_span& operator=(storage_span&&) = default;

  size_t size() const {
    return data_.size();
  }

  T& operator[](size_t i) const {
    return data_[i];
  }

  T& at(size_t i) const {
    return data_.at(i);
  }

  span<T> data() const {
    return data_;
  }

  const std::shared_ptr<vector_type>& storage() const {
    return storage_;
  }

 private:
  std::shared_ptr<vector_type> storage_;
  span<T> data_;
};

}
