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
double span_error(span<const T> sumvec, size_t from, size_t to) {
  // Sum()  = Sum from 'i' to 'n'
  // Vi     = Value at 'i'
  // M      = Mean ... Sum(Vi) / n
  // Error = Sum((Vi - M)^2)
  //       = Sum(Vi^2 + M^2 - 2 * Vi * M)
  //       = Sum(Vi^2) + n * M^2 - 2 * M * Sum(Vi)
  //       = Sum(Vi^2) + M * (n * M - 2 * Sum(Vi))
  //       = Sum(Vi^2) + M * (n * Sum(Vi) / n - 2 * Sum(Vi))
  //       = Sum(Vi^2) - M * Sum(Vi)
  typename T::value_type sum = sumvec[to].sum - sumvec[from].sum;
  typename T::value_type sum2 = sumvec[to].sum2 - sumvec[from].sum2;
  typename T::value_type mean = sum / (to - from);

  // Var(Vi) = Sum((Vi - M)^2) / n
  //         = Sum(Vi^2) / n - M^2
  return static_cast<double>(sum2 / (to - from) - mean * mean);
}

template <typename T>
double split_error(size_t index, span<const T> sumvec) {
  double left_error = span_error(sumvec, 0, index);
  double right_error = span_error(sumvec, index, sumvec.size() - 1);
  double left_weight = static_cast<double>(index) / static_cast<double>(sumvec.size() - 1);

  return left_error * left_weight + right_error * (1.0 - left_weight);
}

}

template <typename T>
std::function<std::optional<split_result> (span<const T>, span<const T>)>
create_splitter(const build_config& bcfg, size_t num_rows, size_t num_columns,
                rnd_generator* rndgen) {
  using accum_type = double;

  struct sum_entry {
    using value_type = accum_type;

    accum_type sum = 0.0;
    accum_type sum2 = 0.0;
  };

  struct context {
    context(size_t num_rows, size_t num_columns) :
        sumvec(num_rows + 1),
        sample_points(num_rows) {
    }

    std::vector<sum_entry> sumvec;
    std::vector<size_t> sample_points;
  };

  std::shared_ptr<context> ctx = std::make_shared<context>(num_rows, num_columns);

  return [&bcfg, rndgen, ctx](span<const T> feat, span<const T> data)
      -> std::optional<split_result> {
    FT_ASSERT(ctx->sumvec.size() >= data.size());

    if (bcfg.min_leaf_size >= data.size()) {
      return std::nullopt;
    }

    size_t margin = bcfg.min_leaf_size / 2;
    size_t left = margin;
    size_t right = (data.size() > margin) ? data.size() - margin : 0;

    while (left < right && (feat[left] - feat.front()) < bcfg.same_eps) {
      ++left;
    }
    if (left >= right) {
      return std::nullopt;
    }

    accum_type sum = 0;
    accum_type sum2 = 0;
    sum_entry* sumvec_ptr = ctx->sumvec.data();

    for (T val : data) {
      accum_type aval = static_cast<accum_type>(val);

      sumvec_ptr->sum = sum;
      sumvec_ptr->sum2 = sum2;
      ++sumvec_ptr;
      sum += aval;
      sum2 += aval * aval;
    }
    sumvec_ptr->sum = sum;
    sumvec_ptr->sum2 = sum2;
    ++sumvec_ptr;

    span<sum_entry> sumvec(ctx->sumvec.data(), sumvec_ptr - ctx->sumvec.data());
    double error = detail::span_error<sum_entry>(sumvec, 0, sumvec.size() - 1);

    std::optional<double> best_score;
    size_t best_index = 0;

    if (bcfg.num_split_points == consts::all) {
      for (size_t i = left; i < right; ++i) {
        double score = error - detail::split_error<sum_entry>(i, sumvec);
        if (!best_score || score > *best_score) {
          best_score = score;
          best_index = i;
        }
      }
    } else {
      span<size_t> sample_points(ctx->sample_points.data(), right - left);

      std::iota(sample_points.begin(), sample_points.end(), left);

      span<size_t> ccs = resample(sample_points, bcfg.num_split_points, rndgen,
                                  /*with_replacement=*/ true);
      for (size_t i : ccs) {
        double score = error - detail::split_error<sum_entry>(i, sumvec);
        if (!best_score || score > *best_score) {
          best_score = score;
          best_index = i;
        }
      }
    }
    if (!best_score || *best_score <= bcfg.min_split_error) {
      return std::nullopt;
    }

    return split_result{best_index, *best_score};
  };
}

}
