#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

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

  using split_fn = std::function<std::optional<split_result<value_type>> (span<const T>)>;

  build_tree_node(const build_config& bcfg, std::shared_ptr<fast_tree::build_data<T>> bdata,
                  set_tree_fn setter_fn, const split_fn& splitter_fn,
                  rnd_generator* rndgen) :
      bcfg_(bcfg),
      bdata_(std::move(bdata)),
      set_fn_(std::move(setter_fn)),
      split_fn_(std::move(splitter_fn)),
      rndgen_(rndgen) {
  }

  const std::shared_ptr<fast_tree::build_data<value_type>>& build_data() const {
    return bdata_;
  }

  std::vector<std::unique_ptr<build_tree_node>> split() const {
    std::optional<split_result<value_type>> best_split;
    std::optional<size_t> best_column;

    std::vector<size_t>
        col_indices = resample(bdata_->data().num_columns(), bcfg_.num_columns, rndgen_);
    for (size_t c: col_indices) {
      std::vector<value_type> col = bdata_->column(c);
      std::optional<split_result<value_type>> sres = split_fn_(col);

      if (sres && (!best_split || sres->score > best_split->score)) {
        best_split = sres;
        best_column = c;
      }
    }

    std::vector<std::unique_ptr<build_tree_node>> leaves;

    if (!best_split) {
      std::unique_ptr<tree_node<value_type>>
          node = std::make_unique<tree_node<value_type>>(bdata_->target());

      set_fn_(std::move(node));
    } else {
      std::vector<span<const size_t>>
          indices = bdata_->split_indices(*best_column, best_split->index);

      if (indices.size() != 2) {
        throw std::runtime_error(string_formatter()
                                 << "Invalid split index " << best_split->index << " / "
                                 << bdata_->data().num_columns());
      }

      std::vector<size_t> left_indices = to_vector(indices[0]);
      std::vector<size_t> right_indices = to_vector(indices[1]);

      std::shared_ptr<fast_tree::build_data<value_type>> left_data =
          std::make_shared<fast_tree::build_data<value_type>>(bdata_, std::move(left_indices));
      std::shared_ptr<fast_tree::build_data<value_type>> right_data =
          std::make_shared<fast_tree::build_data<value_type>>(bdata_, std::move(right_indices));

      std::unique_ptr<tree_node<value_type>>
          node = std::make_unique<tree_node<value_type>>(*best_column, best_split->value);
      tree_node<value_type>* node_ptr = node.get();

      auto left_setter = [node_ptr](std::unique_ptr<tree_node<value_type>> lnode) {
        node_ptr->set_left(std::move(lnode));
      };
      auto right_setter = [node_ptr](std::unique_ptr<tree_node<value_type>> lnode) {
        node_ptr->set_right(std::move(lnode));
      };

      set_fn_(std::move(node));

      leaves.push_back(
          std::make_unique<build_tree_node<value_type>>(bcfg_, std::move(left_data),
                                                        std::move(left_setter), split_fn_,
                                                        rndgen_));
      leaves.push_back(
          std::make_unique<build_tree_node<value_type>>(bcfg_, std::move(right_data),
                                                        std::move(right_setter), split_fn_,
                                                        rndgen_));
    }

    return leaves;
  }

 private:
  const build_config& bcfg_;
  std::shared_ptr<fast_tree::build_data<T>> bdata_;
  set_tree_fn set_fn_;
  const split_fn& split_fn_;
  rnd_generator* rndgen_;
};

}
