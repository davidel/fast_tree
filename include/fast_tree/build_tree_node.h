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
#include "fast_tree/storage_span.h"
#include "fast_tree/string_formatter.h"
#include "fast_tree/tree_node.h"
#include "fast_tree/types.h"
#include "fast_tree/util.h"

namespace fast_tree {

template <typename T>
class build_tree_node {
  struct split_data {
    size_t column;
    T value;
  };

  struct context {
    context(size_t num_rows, size_t num_columns) :
        col_buffer(iota(num_columns)),
        feat_buffer(std::vector<T>(num_rows)),
        tgt_buffer(std::vector<T>(num_rows)) {
    }

    storage_span<size_t> col_buffer;
    storage_span<T> feat_buffer;
    storage_span<T> tgt_buffer;
  };

 public:
  using value_type = T;

  using set_tree_fn = std::function<void (std::unique_ptr<tree_node<T>>)>;

  using split_fn = std::function<std::optional<split_result> (span<const T>,
    span<const T>)>;

  build_tree_node(const build_config& bcfg, std::shared_ptr<build_data<T>> bdata,
                  set_tree_fn setter_fn, const split_fn& splitter_fn, rnd_generator* rndgen) :
      context_(std::make_shared<context>(bdata->data().num_rows(),
                                         bdata->data().num_columns())),
      bcfg_(bcfg),
      bdata_(std::move(bdata)),
      set_fn_(std::move(setter_fn)),
      split_fn_(splitter_fn),
      rndgen_(rndgen) {
  }

  build_tree_node(const build_tree_node& parent, std::shared_ptr<build_data<T>> bdata,
                  set_tree_fn setter_fn) :
      context_(parent.context_),
      bcfg_(parent.bcfg_),
      bdata_(std::move(bdata)),
      set_fn_(std::move(setter_fn)),
      split_fn_(parent.split_fn_),
      rndgen_(parent.rndgen_),
      depth_(parent.depth_ + 1) {
  }

  std::vector<std::unique_ptr<build_tree_node>> split() const {
    std::vector<std::unique_ptr<build_tree_node>> leaves;
    std::optional<split_data> sdata = compute_split();

    if (!sdata) {
      std::unique_ptr<tree_node<T>>
          node = std::make_unique<tree_node<T>>(bdata_->target());

      set_fn_(std::move(node));
    } else {
      size_t part_idx = bdata_->partition_indices(sdata->column, sdata->value);

      std::shared_ptr<build_data<T>> left_data =
          std::make_shared<build_data<T>>(*bdata_, bdata_->start(), part_idx);
      std::shared_ptr<build_data<T>> right_data =
          std::make_shared<build_data<T>>(*bdata_, part_idx, bdata_->end());

      std::unique_ptr<tree_node<T>>
          node = std::make_unique<tree_node<T>>(sdata->column, sdata->value);
      tree_node<T>* node_ptr = node.get();

      set_tree_fn left_setter = [node_ptr](std::unique_ptr<tree_node<T>> lnode) {
        node_ptr->set_left(std::move(lnode));
      };
      set_tree_fn right_setter = [node_ptr](std::unique_ptr<tree_node<T>> rnode) {
        node_ptr->set_right(std::move(rnode));
      };

      set_fn_(std::move(node));

      leaves.push_back(
          std::make_unique<build_tree_node>(*this, std::move(left_data),
                                            std::move(left_setter)));
      leaves.push_back(
          std::make_unique<build_tree_node>(*this, std::move(right_data),
                                            std::move(right_setter)));
    }

    return leaves;
  }

 private:
  std::optional<split_data> compute_split() const {
    if (bcfg_.min_leaf_size >= bdata_->size()) {
      return std::nullopt;
    }

    std::optional<double> best_score;
    std::optional<size_t> best_column;
    std::optional<T> best_value;

    span<size_t> col_samples =
        resample(context_->col_buffer.data(), bcfg_.num_columns, rndgen_);
    for (size_t c: col_samples) {
      typename data<T>::cdata col = bdata_->data().column(c);
      span<size_t> indices = bdata_->indices();

      std::sort(indices.begin(), indices.end(),
                [col](size_t left, size_t right) {
                  return col[left] < col[right];
                });

      // The sort above re-shuffled the indices stored within the build_data,
      // which are used to fetch the column and the target.
      span<T> feat = bdata_->column(c, context_->feat_buffer.data());
      span<T> tgt = bdata_->target(context_->tgt_buffer.data());
      std::optional<split_result> sres = split_fn_(feat, tgt);

      if (sres && (!best_score || sres->score > *best_score)) {
        best_score = sres->score;
        best_column = c;
        best_value = feat[sres->index];
      }
    }
    if (!best_column) {
      return std::nullopt;
    }

    return split_data{*best_column, *best_value};
  }

  std::shared_ptr<context> context_;
  const build_config& bcfg_;
  std::shared_ptr<build_data<T>> bdata_;
  set_tree_fn set_fn_;
  const split_fn& split_fn_;
  rnd_generator* rndgen_;
  size_t depth_ = 0;
};

}
