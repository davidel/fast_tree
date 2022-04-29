#pragma once

#include <cstdint>
#include <memory_resource>
#include <random>
#include <type_traits>
#include <vector>

#include "fast_tree/span.h"

namespace fast_tree {

using bitmap = std::vector<bool>;

using rnd_generator = std::mt19937_64;

template <typename T>
using fvector = std::pmr::vector<T>;

template <typename T>
struct split_result {
  size_t index = 0;
  T value;
  double score = 0.0;
};

}
