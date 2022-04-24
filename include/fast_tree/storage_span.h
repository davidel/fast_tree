#pragma once

#include <memory>
#include <vector>

#include "fast_tree/span.h"
#include "fast_tree/types.h"

namespace fast_tree {

template <typename T>
class storage_span {
 public:
  storage_span(span<const T> data) :
      data_(data) {
  }

  storage_span(std::vector<T>&& storage) :
      storage_(std::make_shared<std::vector<T>>(std::move(storage))),
      data_(*storage_) {
  }

  storage_span(const storage_span& ref) :
      storage_(ref.storage_),
      data_(ref.data()) {
  }

  explicit storage_span(storage_span&& ref) = default;

  storage_span& operator=(const storage_span& ref) {
    if (this != &ref) {
      storage_ = ref.storage_();
      data_ = ref.data();
    }

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
  std::shared_ptr<std::vector<T>> storage_;
  span<const T> data_;
};

}
