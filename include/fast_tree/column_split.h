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

  return left_error + right_error;
}

}

template <typename T>
std::function<std::optional<split_result<T>> (span<const T>)>
create_splitter(const build_config& bcfg, rnd_generator* rndgen) {
  return [&bcfg, rndgen](span<const T> data) -> std::optional<split_result<T>> {
    using accum_type = double;

    accum_type sum = 0;
    std::vector<accum_type> sumvec;

    sumvec.reserve(data.size());
    for (T val : data) {
      sum += static_cast<accum_type>(val);
      sumvec.push_back(sum);
    }
    double error = detail::span_error(data, static_cast<T>(sumvec.back() / data.size()));

    double best_score = -1.0;
    size_t best_index = 0;
    size_t margin = (bcfg.min_leaf_size + 1) / 2;

    for (size_t i = margin; i + margin < data.size(); ++i) {
      while (i + margin + 1 < data.size() && std::abs(data[i + 1] - data[i]) < bcfg.same_eps) {
        ++i;
      }
      if (i + margin < data.size()) {
        double score = error - detail::split_error<T, accum_type>(data, i, sumvec);
        if (score > best_score) {
          best_score = score;
          best_index = i;
        }
      }
    }

    if (best_score <= 0.0) {
      return std::nullopt;
    }

    return split_result<T>{best_index, data[best_index], best_score};
  };
}

}
