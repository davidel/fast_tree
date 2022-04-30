#pragma once

#include <cmath>
#include <functional>
#include <optional>
#include <vector>

#include "fast_tree/build_config.h"
#include "fast_tree/span.h"
#include "fast_tree/types.h"
#include "fast_tree/util.h"

namespace fast_tree {
namespace detail {

template <typename T>
double span_error(span<const T> data, T mean) {
  double error = 0;
  for (T val : data) {
    error += static_cast<double>(std::abs(val - mean));
  }

  return error / data.size();
}

template <typename T, typename S>
double split_error(span<const T> data, size_t index, span<const S> sumvec) {
  size_t n_left = index;
  size_t n_right = data.size() - index;
  T left_mean = static_cast<T>(sumvec[index - 1] / n_left);
  T right_mean = static_cast<T>((sumvec.back() - sumvec[index - 1]) / n_right);
  double left_error = span_error(data.subspan(0, index), left_mean);
  double right_error = span_error(data.subspan(index), right_mean);
  double left_weight = static_cast<double>(index) / static_cast<double>(data.size());

  return left_error * left_weight + right_error * (1.0 - left_weight);
}

}

template <typename T>
std::function<std::optional<split_result> (span<const T>)>
create_splitter(const build_config& bcfg, rnd_generator* rndgen) {
  return [&bcfg, rndgen](span<const T> data) -> std::optional<split_result> {
    using accum_type = double;

    if (bcfg.min_leaf_size >= data.size()) {
      return std::nullopt;
    }

    size_t margin = (bcfg.min_leaf_size + 1) / 2;
    size_t left = margin;
    size_t right = data.size() - margin;

    while (left < right && std::abs(data[left] - data[0]) < bcfg.same_eps) {
      ++left;
    }
    if (left >= right) {
      return std::nullopt;
    }

    accum_type sum = 0;
    std::vector<accum_type> sumvec;

    sumvec.reserve(data.size());
    for (T val : data) {
      sum += static_cast<accum_type>(val);
      sumvec.push_back(sum);
    }
    double error = detail::span_error(data, static_cast<T>(sumvec.back() / data.size()));

    std::optional<double> best_score;
    size_t best_index = 0;

    #if 0

    for (size_t i = left; i < right; ++i) {
      double score = error - detail::split_error<T, accum_type>(data, i, sumvec);
      if (!best_score || score > *best_score) {
        best_score = score;
        best_index = i;
      }
    }

    #else

    std::vector<size_t> ccs = resample(right - left, bcfg.num_split_points, rndgen,
                                       /*with_replacement=*/ true);
    for (size_t x : ccs) {
      size_t i = x + left;
      double score = error - detail::split_error<T, accum_type>(data, i, sumvec);
      if (!best_score || score > *best_score) {
        best_score = score;
        best_index = i;
      }
    }

    #endif

    if (!best_score || *best_score <= bcfg.min_split_error) {
      return std::nullopt;
    }

    return split_result{best_index, *best_score};
  };
}

}
