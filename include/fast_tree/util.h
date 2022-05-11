#pragma once

#include <algorithm>
#include <cstdlib>
#include <numeric>
#include <optional>
#include <random>
#include <stdexcept>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <vector>

#include "fast_tree/assert.h"
#include "fast_tree/constants.h"
#include "fast_tree/span.h"
#include "fast_tree/types.h"

namespace fast_tree {

bitmap create_bitmap(size_t size, span<const size_t> indices);

std::vector<size_t> reduce_indices(span<const size_t> indices, const bitmap& bmap);

std::vector<size_t> iota(size_t size, size_t base = 0);

std::string_view read_line(std::string_view* data);

template<typename T>
std::vector<T> arange(T base, T end, T step = 1) {
  FT_ASSERT(step != 0 &&
            ((end > base && step > 0) || (base > end && step < 0)))
      << "Invalid range " << base << " ... " << end << " with step " << step;

  std::vector<T> values;

  values.reserve(static_cast<size_t>((end - base) / step) + 1);
  if (base <= end) {
    for (T val = base; val < end; val += step) {
      values.push_back(val);
    }
  } else {
    for (T val = base; val > end; val += step) {
      values.push_back(val);
    }
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
std::vector<std::remove_cv_t<T>> take(span<T> vec, span<const size_t> indices) {
  std::vector<std::remove_cv_t<T>> values;

  values.reserve(indices.size());
  for (size_t ix : indices) {
    values.push_back(vec[ix]);
  }

  return values;
}

template<typename T, typename U>
span<U> take(span<T> vec, span<const size_t> indices, span<U> out) {
  FT_ASSERT(indices.size() <= out.size()) << "Buffer too small";
  U* data = out.data();

  for (size_t ix : indices) {
    *data++ = vec[ix];
  }

  return span<U>(out.data(), data - out.data());
}

template<typename T>
std::vector<std::remove_cv_t<T>> to_vector(span<const T> data) {
  return std::vector<std::remove_cv_t<T>>(data.data(), data.data() + data.size());
}

template<typename G>
std::vector<size_t> resample(size_t size, size_t count, G* rgen,
                             bool with_replacement = false) {
  std::vector<size_t> indices;

  if (count == consts::all) {
    indices = iota(size);
  } else {
    size_t ecount = std::min(count, size);
    bitmap mask(size, false);

    indices.reserve(ecount);
    if (with_replacement) {
      bool invert_count = ecount > size / 2;
      size_t xcount = invert_count ? size - ecount : ecount;

      while (xcount > 0) {
        size_t ix = static_cast<size_t>((*rgen)()) % size;
        if (!mask[ix]) {
          mask[ix] = true;
          --xcount;
        }
      }
      for (size_t i = 0; i < size; ++i) {
        if (mask[i] ^ invert_count) {
          indices.push_back(i);
        }
      }
    } else {
      for (size_t i = 0; i < ecount; ++i) {
        size_t ix = static_cast<size_t>((*rgen)()) % size;
        mask[ix] = true;
      }
      for (size_t i = 0; i < size; ++i) {
        if (mask[i]) {
          indices.push_back(i);
        }
      }
    }
  }

  return indices;
}

template<typename G>
span<size_t> resample(span<size_t> in_indices, size_t count, G* rgen,
                      bool with_replacement = false) {
  span<size_t> indices;

  if (count == consts::all || count >= in_indices.size()) {
    indices = in_indices;
  } else {
    if (with_replacement) {
      size_t rspace = in_indices.size() - 1;

      for (size_t i = 0; i < count; ++i, --rspace) {
        size_t ix = static_cast<size_t>((*rgen)()) % rspace;

        std::swap(in_indices[i], in_indices[i + ix + 1]);
      }
      indices = in_indices.subspan(0, count);
    } else {
      size_t n = 0;

      for (size_t i = 0; i < count; ++i) {
        size_t ix = static_cast<size_t>((*rgen)()) % in_indices.size();

        if (ix > n) {
          std::swap(in_indices[n], in_indices[ix]);
          ++n;
        }
      }
      indices = in_indices.subspan(0, n);
    }
  }

  return indices;
}

static inline void copy_buffer(std::string_view vdata, char* buffer, size_t size) {
  FT_ASSERT(vdata.size() < size)
      << "String size too big for destination buffer: "
      << vdata.size() << " vs. " << size;

  // std::memcpy() is slower than this code for small buffers (we are usually
  // copying an handfull of bytes here) as it does a bunch of check on entry
  // to figure out if it can use xmm copies.
  const char* dptr = vdata.data();
  const char* dend = dptr + vdata.size();
  char* ptr = buffer;

  while (dptr < dend) {
    *ptr++ = *dptr++;
  }
  *ptr = '\0';
}

template<typename U,
         typename std::enable_if<std::is_integral<U>::value>::type* = nullptr>
std::optional<U> from_chars(std::string_view vdata) {
  // std::from_chars() is not fully implemented in clang.
  char buffer[128];

  copy_buffer(vdata, buffer, sizeof(buffer));
  char* eob = buffer;
  auto value = std::strtoll(buffer, &eob, 10);

  if (eob == buffer) {
    return std::nullopt;
  }

  return static_cast<U>(value);
}

template<class U,
         typename std::enable_if<std::is_floating_point<U>::value>::type* = nullptr>
std::optional<U> from_chars(std::string_view vdata) {
  // std::from_chars() is not fully implemented in clang.
  char buffer[128];

  copy_buffer(vdata, buffer, sizeof(buffer));
  char* eob = buffer;
  auto value = std::strtold(buffer, &eob);

  if (eob == buffer) {
    return std::nullopt;
  }

  return static_cast<U>(value);
}

// User defined for non standard types.
template <typename S>
std::optional<S> from_string_view(std::string_view);

template<class U,
         typename std::enable_if<!std::is_arithmetic<U>::value>::type* = nullptr>
std::optional<U> from_chars(std::string_view vdata) {
  return from_string_view<U>(vdata);
}

}
