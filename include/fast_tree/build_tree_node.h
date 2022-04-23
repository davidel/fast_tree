#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "fast_tree/build_data.h"
#include "fast_tree/span.h"
#include "fast_tree/tree_node.h"
#include "fast_tree/types.h"

namespace fast_tree {

template <typename T>
class build_tree_node {
 public:
  using value_type = T;

  using set_tree_fn = std::function<void (std::unique_ptr<tree_node<T>>)>;

  build_tree_node(std::unique_ptr<build_data<T>> bdata, set_tree_fn set_fn) :
      bdata_(std::move(bdata)),
      set_fn_(std::move(set_fn)) {
  }

 private:
  std::unique_ptr<build_data<T>> bdata_;
  set_tree_fn set_fn_;
};

}
