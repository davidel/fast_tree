#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

#include "fast_tree/assert.h"
#include "fast_tree/build_config.h"
#include "fast_tree/build_data.h"
#include "fast_tree/span.h"
#include "fast_tree/string_formatter.h"
#include "fast_tree/tree_node.h"
#include "fast_tree/types.h"
#include "fast_tree/util.h"

namespace fast_tree {

template <typename T>
class build_tree_node {
 public:
  using value_type = T;

  using set_tree_fn = std::function<void (std::unique_ptr<tree_node<T>>)>;

  using split_fn = std::function<std::optional<split_result<T>> (span<const T>)>;

  build_tree_node(const build_config& bcfg, std::shared_ptr<fast_tree::build_data<T>> bdata,
                  set_tree_fn setter_fn, const split_fn& splitter_fn, rnd_generator* rndgen,
                  size_t depth = 0) :
      bcfg_(bcfg),
      bdata_(std::move(bdata)),
      set_fn_(std::move(setter_fn)),
      split_fn_(std::move(splitter_fn)),
      rndgen_(rndgen),
      depth_(depth) {
  }

  std::vector<std::unique_ptr<build_tree_node>> split() const {
    std::optional<split_result<T>> best_split;
    std::optional<size_t> best_column;

    storage_span<T> col_buffer(std::vector<T>(bdata_->data().num_rows()));
    std::vector<size_t>
        col_indices = resample(bdata_->data().num_columns(), bcfg_.num_columns, rndgen_);
    for (size_t c: col_indices) {
      span<T> col = bdata_->column(c, col_buffer.data());

      std::sort(col.begin(), col.end());

      std::optional<split_result<T>> sres = split_fn_(col);

      if (sres && (!best_split || sres->score > best_split->score)) {
        best_split = sres;
        best_column = c;
      }
    }

    std::vector<std::unique_ptr<build_tree_node>> leaves;

    if (!best_split) {
      std::unique_ptr<tree_node<T>>
          node = std::make_unique<tree_node<T>>(bdata_->target());

      set_fn_(std::move(node));
    } else {
      size_t split_idx = bdata_->split_indices(*best_column, best_split->value);

      std::shared_ptr<fast_tree::build_data<T>> left_data =
          std::make_shared<fast_tree::build_data<T>>(*bdata_, bdata_->start(), split_idx);
      std::shared_ptr<fast_tree::build_data<T>> right_data =
          std::make_shared<fast_tree::build_data<T>>(*bdata_, split_idx, bdata_->end());

      std::unique_ptr<tree_node<T>>
          node = std::make_unique<tree_node<T>>(*best_column, best_split->value);
      tree_node<T>* node_ptr = node.get();

      auto left_setter = [node_ptr](std::unique_ptr<tree_node<T>> lnode) {
        node_ptr->set_left(std::move(lnode));
      };
      auto right_setter = [node_ptr](std::unique_ptr<tree_node<T>> rnode) {
        node_ptr->set_right(std::move(rnode));
      };

      set_fn_(std::move(node));

      leaves.push_back(
          std::make_unique<build_tree_node<T>>(bcfg_, std::move(left_data),
                                               std::move(left_setter), split_fn_,
                                               rndgen_, /*depth=*/ depth_ + 1));
      leaves.push_back(
          std::make_unique<build_tree_node<T>>(bcfg_, std::move(right_data),
                                               std::move(right_setter), split_fn_,
                                               rndgen_, /*depth=*/ depth_ + 1));
    }

    return leaves;
  }

 private:
  const build_config& bcfg_;
  std::shared_ptr<fast_tree::build_data<T>> bdata_;
  set_tree_fn set_fn_;
  const split_fn& split_fn_;
  rnd_generator* rndgen_;
  size_t depth_;
};

}
