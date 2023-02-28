#pragma once

#include <cstdint>

namespace fast_tree {

struct split_result {
  std::size_t index = 0;
  double score = 0.0;
};

}
