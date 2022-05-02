#pragma once

#include <memory>
#include <vector>

#include "fast_tree/span.h"
#include "fast_tree/types.h"

namespace fast_tree {

template <typename T>
class tree_node {
 public:
  using value_type = T;

  static constexpr size_t no_index = static_cast<size_t>(-1);

  explicit tree_node(std::vector<T> values) :
      splitter_(),
      values_(std::move(values)) {
  }

  tree_node(size_t index, const T& splitter) :
      index_(index),
      splitter_(splitter) {
  }

  bool is_leaf() const {
    return index_ == no_index;
  }

  size_t index() const {
    return index_;
  }

  const T& splitter() const {
    return splitter_;
  }

  span<const T> values() const {
    return values_;
  }

  const tree_node& left() const {
    return *left_;
  }

  const tree_node& right() const {
    return *right_;
  }

  void set_left(std::unique_ptr<tree_node> left_ptr) {
    left_ = std::move(left_ptr);
  }

  void set_right(std::unique_ptr<tree_node> right_ptr) {
    right_ = std::move(right_ptr);
  }

  span<const T> eval(const span<const T>& row) const {
    const tree_node* node = this;

    while (!node->is_leaf()) {
      T row_value = row.at(node->index());

      if (row_value < node->splitter()) {
        node = &left();
      } else {
        node = &right();
      }
    }

    return node->values_;
  }

 private:
  size_t index_ = no_index;
  T splitter_;
  std::vector<T> values_;
  std::unique_ptr<tree_node> left_;
  std::unique_ptr<tree_node> right_;
};

}
