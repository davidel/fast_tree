#pragma once

#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

#include "fast_tree/assert.h"
#include "fast_tree/tree_node.h"
#include "fast_tree/types.h"
#include "fast_tree/util.h"

namespace fast_tree {

template <typename T>
class forest {
  static constexpr std::string_view forest_begin = std::string_view("FOREST BEGIN");
  static constexpr std::string_view forest_end = std::string_view("FOREST END");

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

  void store(std::ostream* stream) const {
    (*stream) << forest_begin << "\n";

    for (auto& tree : trees_) {
      tree->store(stream);
    }

    (*stream) << forest_end << "\n";
  }

  static std::unique_ptr<forest> load(std::string_view* data) {
    std::string_view remaining = *data;
    std::string_view ln = read_line(&remaining);

    FT_ASSERT(ln == forest_begin) << "Invalid forest open statement: " << ln;

    std::vector<std::unique_ptr<tree_node<T>>> trees;

    while (!remaining.empty()) {
      std::string_view peeksv = remaining;

      ln = read_line(&peeksv);
      if (ln == forest_end) {
        remaining = peeksv;
        break;
      }

      trees.push_back(tree_node<T>::load(&remaining));
    }
    FT_ASSERT(ln == forest_end)
        << "Unbale to find forest end statement (\"" << forest_end << "\")";

    *data = remaining;

    return std::make_unique<forest>(std::move(trees));
  }

 private:
  std::vector<std::unique_ptr<tree_node<T>>> trees_;
};

}
