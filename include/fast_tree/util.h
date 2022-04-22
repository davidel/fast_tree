#pragma once

#include <algorithm>
#include <numeric>
#include <type_traits>
#include <vector>

#include "fast_tree/span.h"
#include "fast_tree/types.h"

namespace fast_tree {
namespace consts {

static constexpr size_t all_indices = static_cast<size_t>(-1);

}

std::vector<size_t> reduce_indices(span<size_t> indices, const bitmap& bmap);

template<typename T>
std::vector<size_t> argsort(const T& array, bool descending = false) {
  std::vector<size_t> indices(array.size());

  std::iota(indices.begin(), indices.end(), 0);
  if (descending) {
    std::sort(indices.begin(), indices.end(),
              [&array](size_t left, size_t right) {
                return array[left] > array[right];
              });
  } else {
    std::sort(indices.begin(), indices.end(),
              [&array](size_t left, size_t right) {
                return array[left] < array[right];
              });
  }

  return indices;
}

template<typename T>
std::vector<std::remove_cv_t<T>> to_vector(span<T> data) {
  return std::vector<std::remove_cv_t<T>>(data.data(), data.data() + data.size());
}

template<typename G>
std::vector<size_t> resample(size_t size, size_t count, G& rgen) {
  std::vector<size_t> indices;

  if (count == consts::all_indices) {
    indices.resize(size);
    std::iota(indices.begin(), indices.end(), 0);
  } else {
    std::vector<bool> mask(size, false);

    for (size_t i = 0; i < count; ++i) {
      size_t ix = static_cast<size_t>(rgen()) % size;
      mask[ix] = true;
    }

    indices.reserve(count);
    for (size_t i = 0; i < size; ++i) {
      if (mask[i]) {
        indices.push_back(i);
      }
    }
  }

  return indices;
}

}
