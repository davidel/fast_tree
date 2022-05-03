#pragma once

#include <memory>
#include <vector>

#include "fast_tree/column_split.h"
#include "fast_tree/build_config.h"
#include "fast_tree/build_data.h"
#include "fast_tree/build_tree_node.h"
#include "fast_tree/data.h"
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
std::vector<std::unique_ptr<tree_node<T>>>
build_forest(const build_config& bcfg, std::shared_ptr<build_data<T>> bdata, size_t num_trees,
             rnd_generator* rndgen, size_t num_threads = 0) {
  std::vector<std::unique_ptr<tree_node<T>>> forest;

  if (num_threads == 1) {
    forest.reserve(num_trees);
    for (size_t i = 0; i < num_trees; ++i) {
      forest.push_back(build_tree(bcfg, detail::generate_build_data(bcfg, bdata, rndgen),
                                  rndgen));
    }
  } else {
    std::vector<std::shared_ptr<build_data<T>>> trees_data;

    trees_data.reserve(num_trees);
    for (size_t i = 0; i < num_trees; ++i) {
      trees_data.push_back(detail::generate_build_data(bcfg, bdata, rndgen));
    }

    std::function<std::unique_ptr<tree_node<T>> (const std::shared_ptr<build_data<T>>&)>
        build_fn = [&bcfg, rndgen](const std::shared_ptr<build_data<T>>& tdata)
        -> std::unique_ptr<tree_node<T>> {
      return build_tree(bcfg, tdata, rndgen);
    };

    forest = map(build_fn, trees_data.begin(), trees_data.end(),
                 /*num_threads=*/ effective_num_threads(num_threads, num_trees));
  }

  return forest;
}

}
