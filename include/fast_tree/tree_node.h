#pragma once

#include <iostream>
#include <memory>
#include <vector>

#include "fast_tree/assert.h"
#include "fast_tree/constants.h"
#include "fast_tree/span.h"
#include "fast_tree/types.h"

namespace fast_tree {

template <typename T>
class tree_node {
 public:
  using value_type = T;

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
    return index_ == consts::invalid_index;
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

  void store(std::ostream* stream) const {
    struct entry {
      entry(const tree_node* node, size_t parent_idx) :
          node(node),
          parent_idx(parent_idx) {
      }

      const tree_node* node = nullptr;
      size_t parent_idx = consts::invalid_index;
      size_t left_idx = consts::invalid_index;;
      size_t right_idx = consts::invalid_index;
    };

    std::vector<std::unique_ptr<entry>> stack;

    stack.emplace_back(std::make_unique<entry>(this, 0));
    for (size_t rindex = 0; stack.size() > rindex; ++rindex) {
      entry* ent = stack[rindex].get();

      if (!ent->node->is_leaf()) {
        stack[ent->parent_idx]->left_idx = stack.size();
        stack.emplace_back(std::make_unique<entry>(ent->node->left(), stack.size()));

        stack[ent->parent_idx]->right_idx = stack.size();
        stack.emplace_back(std::make_unique<entry>(ent->node->right(), stack.size()));
      }
    }

    for (size_t i = stack.size(); i > 0; --i) {
      const entry& ent = *stack[i - 1];

      (*stream) << (i - 1);
      if (ent.node->is_leaf()) {
        FT_ASSERT(ent.left_idx == consts::invalid_index) << ent.left_idx;
        FT_ASSERT(ent.right_idx == consts::invalid_index) << ent.right_idx;

        (*stream) << " * *";
        for (T value : ent.node->values()) {
          (*stream) << " " << value;
        }
      } else {
        FT_ASSERT(ent.left_idx != consts::invalid_index);
        FT_ASSERT(ent.right_idx != consts::invalid_index);

        (*stream) << " " << ent.left_idx << " " << ent.right_idx
                  << " " << ent.node->index() << " " << ent.node->splitter();
      }
      (*stream) << "\n";
    }
  }

 private:
  size_t index_ = consts::invalid_index;
  T splitter_;
  std::vector<T> values_;
  std::unique_ptr<tree_node> left_;
  std::unique_ptr<tree_node> right_;
};

}
