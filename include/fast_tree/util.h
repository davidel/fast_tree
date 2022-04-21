#pragma once

#include <algorithm>
#include <numeric>
#include <vector>

namespace fast_tree {

template<typename T>
std::vector<size_t> argsort(const T& array) {
  std::vector<size_t> indices(array.size());

  std::iota(indices.begin(), indices.end(), 0);
  std::sort(indices.begin(), indices.end(),
            [&array](size_t left, size_t right) {
              return array[left] < array[right];
            });

  return indices;
}

}
