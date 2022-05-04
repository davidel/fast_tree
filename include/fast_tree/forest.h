#pragma once

#include <memory>
#include <vector>

#include "fast_tree/assert.h"
#include "fast_tree/tree_node.h"
#include "fast_tree/types.h"

namespace fast_tree {

template <typename T>
class forest {
 public:
  using value_type = T;

  explicit forest(std::vector<std::unique_ptr<tree_node<T>>>&& trees) :
      trees_(std::move(trees)) {
  }

  size_t size() const {
    return trees_.size();
  }

  const tree_node<T>& operator[](size_t i) const {
    return *trees_[i];
  }

  std::vector<span<const T>> eval(span<const T> row) const {
    std::vector<span<const T>> results;

    results.reserve(trees_.size());
    for (auto& tree : trees_) {
      results.push_back(tree->eval(row));
    }

    return results;
  }

 private:
  std::vector<std::unique_ptr<tree_node<T>>> trees_;
};

}
