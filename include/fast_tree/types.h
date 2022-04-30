#pragma once

#include <cstdint>
#include <random>
#include <type_traits>
#include <vector>

#include "fast_tree/span.h"

namespace fast_tree {

using bitmap = std::vector<bool>;

using rnd_generator = std::mt19937_64;

struct split_result {
  size_t index = 0;
  double score = 0.0;
};

}
