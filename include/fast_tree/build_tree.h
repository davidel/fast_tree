#pragma once

#include <memory>
#include <vector>

#include "fast_tree/build_config.h"
#include "fast_tree/build_data.h"
#include "fast_tree/build_tree_node.h"
#include "fast_tree/column_split.h"
#include "fast_tree/data.h"
#include "fast_tree/forest.h"
#include "fast_tree/threadpool.h"
#include "fast_tree/tree_node.h"
#include "fast_tree/types.h"
#include "fast_tree/util.h"

namespace fast_tree {
namespace detail {

template <typename T>
std::shared_ptr<build_data<T>> generate_build_data(
    const build_config& bcfg, const std::shared_ptr<build_data<T>>& bdata,
    rnd_generator* rndgen) {
  std::vector<size_t>
      row_indices = resample(bdata->data().num_rows(), bcfg.num_rows, rndgen);

  return std::make_shared<build_data<T>>(bdata->data(), std::move(row_indices));
}

}

template <typename T>
std::unique_ptr<tree_node<T>> build_tree(const build_config& bcfg,
                                         std::shared_ptr<build_data<T>> bdata,
                                         rnd_generator* rndgen) {
  std::unique_ptr<tree_node<T>> root;

  typename build_tree_node<T>::set_tree_fn
      setter = [&root](std::unique_ptr<tree_node<T>> node) {
    root = std::move(node);
  };

  typename build_tree_node<T>::split_fn splitter = create_splitter<T>(bcfg, rndgen);
  std::vector<std::unique_ptr<build_tree_node<T>>> queue;

  queue.push_back(std::make_unique<build_tree_node<T>>(
      bcfg, std::move(bdata), std::move(setter), splitter, rndgen));

  while (!queue.empty()) {
    std::vector<std::unique_ptr<build_tree_node<T>>> split = queue.back()->split();

    queue.pop_back();
    for (size_t i = 0; i < split.size(); ++i) {
      queue.push_back(std::move(split[i]));
    }
  }

  return root;
}

template <typename T>
forest<T> build_forest(const build_config& bcfg, std::shared_ptr<build_data<T>> bdata,
                       size_t num_trees, rnd_generator* rndgen,
                       size_t num_threads = 0) {
  std::vector<std::unique_ptr<tree_node<T>>> trees;

  if (num_threads == 1) {
    trees.reserve(num_trees);
    for (size_t i = 0; i < num_trees; ++i) {
      trees.push_back(build_tree(bcfg, detail::generate_build_data(bcfg, bdata, rndgen),
                                 rndgen));
    }
  } else {
    struct tree_build_context {
      tree_build_context(std::shared_ptr<build_data<T>> bdata, rnd_generator* rgen) :
          bdata(std::move(bdata)),
          rndgen((*rgen)()) {
      }

      std::shared_ptr<build_data<T>> bdata;
      rnd_generator rndgen;
    };

    std::vector<tree_build_context> trees_ctxs;

    trees_ctxs.reserve(num_trees);
    for (size_t i = 0; i < num_trees; ++i) {
      trees_ctxs.emplace_back(detail::generate_build_data(bcfg, bdata, rndgen), rndgen);
    }

    std::function<std::unique_ptr<tree_node<T>> (tree_build_context&)>
        build_fn = [&bcfg, rndgen](tree_build_context& tctx)
        -> std::unique_ptr<tree_node<T>> {
      return build_tree(bcfg, tctx.bdata, &tctx.rndgen);
    };

    trees = map(build_fn, trees_ctxs.begin(), trees_ctxs.end(),
                /*num_threads=*/ effective_num_threads(num_threads, num_trees));
  }

  return forest<T>(std::move(trees));
}

}
