#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "fast_tree/constants.h"
#include "fast_tree/span.h"
#include "fast_tree/types.h"

namespace fast_tree {

struct build_config {
  size_t num_rows = consts::all;
  size_t num_columns = consts::all;
  size_t min_leaf_size = 4;
  size_t max_depth = consts::all;
  size_t num_split_points = 10;
  double min_split_error = 0.0;
  double same_eps = 1e-6;
  size_t scorer_window = 20;
  double scorer_threshold_pct = 0.75;
};

}
