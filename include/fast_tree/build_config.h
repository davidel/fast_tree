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
};

}
