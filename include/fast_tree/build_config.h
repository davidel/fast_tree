#pragma once

#include <cstdint>

#include "dcpl/constants.h"

namespace fast_tree {

struct build_config {
  std::size_t num_rows = dcpl::consts::all;
  std::size_t num_columns = dcpl::consts::all;
  std::size_t min_leaf_size = 4;
  std::size_t max_depth = dcpl::consts::all;
  std::size_t num_split_points = 10;
  double min_split_error = 0.0;
  double same_eps = 1e-6;
};

}
