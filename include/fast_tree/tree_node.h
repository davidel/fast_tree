#pragma once

#include <memory>
#include <vector>

#include "fast_tree/assert.h"
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

  tree_node(const tree_node&) = delete;

  tree_node(tree_node&&) = delete;

  tree_node& operator=(const tree_node&) = delete;

  tree_node& operator=(tree_node&&) = delete;

  bool is_leaf() const {
    return index_ == no_index;
  }

  size_t index() const {
    return index_;
  }

  T splitter() const {
    return splitter_;
  }

  span<const T> values() const {
    return values_;
  }

  const tree_node* left() const {
    return left_.get();
  }

  const tree_node* right() const {
    return right_.get();
  }

  void set_left(std::unique_ptr<tree_node> node) {
    left_ = std::move(node);
  }

  void set_right(std::unique_ptr<tree_node> node) {
    right_ = std::move(node);
  }

  span<const T> eval(span<const T> row) const {
    const tree_node* node = this;

    while (!node->is_leaf()) {
      T row_value = row.at(node->index());

      if (row_value < node->splitter()) {
        node = node->left();
      } else {
        node = node->right();
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
