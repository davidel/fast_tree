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
std::function<std::optional<split_result> (span<const T>, span<const T>)>
create_splitter(const build_config& bcfg, size_t num_rows, size_t num_columns,
                rnd_generator* rndgen) {
  using accum_type = double;

  struct context {
    context(size_t num_rows, size_t num_columns) :
        sumvec(num_rows),
        sample_points(num_rows) {
    }

    std::vector<accum_type> sumvec;
    std::vector<size_t> sample_points;
  };

  std::shared_ptr<context> ctx = std::make_shared<context>(num_rows, num_columns);

  return [&bcfg, rndgen, ctx](span<const T> feat, span<const T> data)
      -> std::optional<split_result> {
    if (bcfg.min_leaf_size >= data.size()) {
      return std::nullopt;
    }

    size_t margin = std::max<size_t>(bcfg.min_leaf_size / 2, 1);
    size_t left = margin;
    size_t right = (data.size() > margin) ? data.size() - margin : 0;

    while (left < right && std::abs(feat[left] - feat.front()) < bcfg.same_eps) {
      ++left;
    }
    if (left >= right) {
      return std::nullopt;
    }

    accum_type sum = 0;
    accum_type* sptr = ctx->sumvec.data();

    FT_ASSERT(ctx->sumvec.size() >= data.size());

    for (T val : data) {
      sum += static_cast<accum_type>(val);
      *sptr++ = sum;
    }
    span<accum_type> sumvec(ctx->sumvec.data(), sptr - ctx->sumvec.data());
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

    span<size_t> sample_points(ctx->sample_points.data(), right - left);

    std::iota(sample_points.begin(), sample_points.end(), left);

    span<size_t> ccs = resample(sample_points, bcfg.num_split_points, rndgen,
                                /*with_replacement=*/ true);
    for (size_t i : ccs) {
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
