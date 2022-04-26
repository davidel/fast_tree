#pragma once

#include <algorithm>
#include <numeric>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "fast_tree/constants.h"
#include "fast_tree/span.h"
#include "fast_tree/string_formatter.h"
#include "fast_tree/types.h"

namespace fast_tree {

bitmap create_bitmap(size_t size, span<const size_t> indices);

std::vector<size_t> reduce_indices(span<const size_t> indices, const bitmap& bmap);

std::vector<size_t> iota(size_t size, size_t base = 0);

template<typename T>
std::vector<T> arange(T base, T end, T step = 1) {
  if ((end > base && step <= 0) || (base > end && step >= 0)) {
    throw std::invalid_argument(string_formatter()
                                << "Invalid range " << base << " ... " << end
                                << " with step " << step);
  }

  std::vector<T> values;

  values.reserve(static_cast<size_t>((end - base) / step) + 1);
  for (T val = base; val < end; val += step) {
    values.push_back(val);
  }

  return values;
}

template<typename T, typename G>
std::vector<T> randn(size_t count, G* rgen, T rmin = 0, T rmax = 1) {
  std::uniform_real_distribution<T> gen(rmin, rmax);
  std::vector<T> values;

  values.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    values.push_back(gen(*rgen));
  }

  return values;
}

template<typename T>
std::vector<size_t> argsort(const T& array, bool descending = false) {
  std::vector<size_t> indices = iota(array.size());

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
std::vector<T> take(span<const T> vec, span<const size_t> indices) {
  std::vector<T> values;

  values.reserve(indices.size());
  for (size_t ix : indices) {
    values.push_back(vec[ix]);
  }

  return values;
}

template<typename T>
std::vector<std::remove_cv_t<T>> to_vector(span<const T> data) {
  return std::vector<std::remove_cv_t<T>>(data.data(), data.data() + data.size());
}

template<typename G>
std::vector<size_t> resample(size_t size, size_t count, G* rgen) {
  std::vector<size_t> indices;

  if (count == consts::all || count >= size) {
    indices = iota(size);
  } else {
    std::vector<bool> mask(size, false);

    for (size_t i = 0; i < count; ++i) {
      size_t ix = static_cast<size_t>((*rgen)()) % size;
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
