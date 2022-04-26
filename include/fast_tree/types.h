#pragma once

#include <cstdint>
#include <type_traits>
#include <vector>

#include "fast_tree/span.h"

namespace fast_tree {

using bitmap = std::vector<bool>;

template <typename T>
struct split_result {
  size_t index = 0;
  T value;
  double score = 0.0;
};

}
