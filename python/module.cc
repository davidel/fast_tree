#include <algorithm>
#include <optional>

#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "fast_tree/build_config.h"
#include "fast_tree/build_tree.h"
#include "fast_tree/data.h"
#include "fast_tree/forest.h"
#include "fast_tree/span.h"
#include "fast_tree/storage_span.h"
#include "fast_tree/tree_node.h"
#include "fast_tree/types.h"
#include "fast_tree/util.h"

namespace py = pybind11;

namespace fast_tree {
namespace pymod {

template <typename T>
std::optional<T> get_value(const py::dict& dict, const char* name) {
  py::str yname(name);

  if (!dict.contains(yname)) {
    return std::nullopt;
  }

  return dict[yname].cast<T>();
}

template <typename T>
T get_value_or(const py::dict& dict, const char* name, const T& defval) {
  std::optional<T> value = get_value<T>(dict, name);

  return value.value_or(defval);
}

template <typename T>
T get_partial(T size, const py::dict& opts, const char* name, T defval) {
  py::str yname(name);

  if (!opts.contains(yname)) {
    return defval;
  }

  T value{};
  auto opt_value = opts[yname];

  if (py::isinstance<py::int_>(opt_value)) {
    value = std::min<T>(size, opt_value.cast<int>());
  } else if (py::isinstance<py::float_>(opt_value)) {

  } else {
    value = std::min<T>(size, static_cast<T>(size * opt_value.cast<float>()));
  }

  return value;
}

build_config get_build_config(size_t num_rows, size_t num_columns,
                              const py::dict& opts) {
  build_config bcfg;

  bcfg.num_rows = get_partial<size_t>(num_rows, opts, "max_rows", bcfg.num_rows);
  bcfg.num_columns = get_partial<size_t>(num_columns, opts, "max_columns", bcfg.num_columns);
  bcfg.min_leaf_size = get_value_or<size_t>(opts, "min_leaf_size", bcfg.min_leaf_size);
  bcfg.num_split_points = get_value_or<size_t>(opts, "num_split_points", bcfg.num_split_points);
  bcfg.min_split_error = get_value_or<double>(opts, "min_split_error", bcfg.min_split_error);
  bcfg.same_eps = get_value_or<double>(opts, "same_eps", bcfg.same_eps);

  return bcfg;
}

size_t create_forest(py::dict opts) {
  // HACK!
  size_t num_rows = 100;
  size_t num_columns = 10;
  build_config bcfg = get_build_config(num_rows, num_columns, opts);


  return 17;
}

}
}

PYBIND11_MODULE(fast_tree_pylib, mod) {

  mod.def("create_forest",
          &fast_tree::pymod::create_forest,
          py::arg("opts") = py::dict());
}
