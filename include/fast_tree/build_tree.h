#pragma once

#include <deque>
#include <memory>
#include <vector>

#include "fast_tree/column_split.h"
#include "fast_tree/build_config.h"
#include "fast_tree/build_data.h"
#include "fast_tree/build_tree_node.h"
#include "fast_tree/data.h"
#include "fast_tree/tree_node.h"
#include "fast_tree/types.h"

namespace fast_tree {

template <typename T>
std::unique_ptr<tree_node<T>> build_tree(const build_config& bcfg, const data<T>& rdata,
                                         rnd_generator* rndgen) {
  std::unique_ptr<build_data<T>> bdata = std::make_unique<build_data<T>>(rdata);
  std::unique_ptr<tree_node<T>> root;

  auto setter = [&](std::unique_ptr<tree_node<T>> node) {
    root = std::move(node);
  };

  typename build_tree_node<T>::split_fn splitter = create_splitter<T>(bcfg, rndgen);
  std::deque<std::unique_ptr<build_tree_node<T>>> queue;

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
build_forest(const build_config& bcfg, const data<T>& rdata, size_t num_trees,
             rnd_generator* rndgen) {
  std::vector<std::unique_ptr<tree_node<T>>> forest;

  if ((bcfg.num_rows == consts::all || bcfg.num_rows >= rdata.num_rows()) &&
      (bcfg.num_columns == consts::all || bcfg.num_columns >= rdata.num_columns())) {
    num_trees = 1;
  }

  forest.reserve(num_trees);
  for (size_t i = 0; i < num_trees; ++i) {
    std::unique_ptr<data<T>> sdata;
    const data<T>* current_data = &rdata;

    if (bcfg.num_rows != consts::all && bcfg.num_rows < rdata.num_rows()) {
      sdata = rdata.resample(bcfg.num_rows, rndgen);
      current_data = sdata.get();
    }

    forest.push_back(build_tree(bcfg, *current_data, rndgen));
  }

  return forest;
}

}
