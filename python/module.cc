#include <algorithm>
#include <memory>
#include <optional>
#include <stdexcept>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "fast_tree/assert.h"
#include "fast_tree/build_config.h"
#include "fast_tree/build_tree.h"
#include "fast_tree/data.h"
#include "fast_tree/forest.h"
#include "fast_tree/mapfile.h"
#include "fast_tree/span.h"
#include "fast_tree/storage_span.h"
#include "fast_tree/string_formatter.h"
#include "fast_tree/tree_node.h"
#include "fast_tree/types.h"
#include "fast_tree/util.h"

namespace py = pybind11;

namespace fast_tree {
namespace pymod {

using ft_type = float;

using arr_type = py::array_t<ft_type, py::array::c_style | py::array::forcecast>;

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
    value = std::min<T>(size, static_cast<T>(size * opt_value.cast<double>()));
  } else {
    throw std::invalid_argument(
        string_formatter() << "Invalid type for \"" << name
        << "\" option (must be int or float)");
  }

  return value;
}

span<ft_type> array_span(const arr_type& arr) {
  FT_ASSERT(arr.ndim() == 1) << "Input has multi-dimensional shape: " <<
      span(arr.shape(), arr.ndim());

  // We const-cast but it is safe as the forest/tree API never writes into the buffers.
  return span<ft_type>(const_cast<ft_type*>(arr.data()), arr.size());
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

template <typename T>
struct py_forest {
  explicit py_forest(std::unique_ptr<forest<T>> forest) :
      forest(std::move(forest)) {
  }

  size_t size() const {
    return forest->size();
  }

  std::string str(int precision) const {
    std::stringstream ss;

    forest->store(&ss, /*precision=*/ precision);

    return ss.str();
  }

  std::vector<arr_type> eval(const arr_type& data) const {
    auto adata = data.unchecked<2>();
    std::vector<arr_type> result;

    result.reserve(data.shape(0));

    std::vector<ft_type> row(data.shape(1));

    for (size_t i = 0; i < data.shape(0); ++i) {

      for (size_t j = 0; j < data.shape(1); ++j) {
        row[j] = adata(i, j);
      }

      std::vector<span<const ft_type>> rres = forest->eval(row);
      size_t rsize = 0;

      for (span<const ft_type>& s : rres) {
        rsize += s.size();
      }

      arr_type rarr(arr_type::ShapeContainer{rsize});
      auto ares = rarr.mutable_unchecked<1>();
      size_t x = 0;

      for (span<const ft_type>& s : rres) {
        for (ft_type v : s) {
          ares(x) = v;
          ++x;
        }
      }

      result.push_back(std::move(rarr));
    }

    return result;
  }

  std::unique_ptr<forest<T>> forest;
};

std::unique_ptr<py_forest<ft_type>> create_forest(
    const std::vector<arr_type>& columns, arr_type target, size_t num_trees, py::dict opts,
    size_t seed, size_t num_threads) {
  build_config bcfg = get_build_config(target.size(), columns.size(), opts);

  std::unique_ptr<data<ft_type>>
      rdata = std::make_unique<data<ft_type>>(array_span(target));

  for (auto& col : columns) {
    rdata->add_column(array_span(col));
  }

  std::shared_ptr<build_data<ft_type>>
      bdata = std::make_shared<build_data<ft_type>>(*rdata);
  rnd_generator gen(seed);

  py::gil_scoped_release release;

  std::unique_ptr<forest<ft_type>>
      forest = build_forest(bcfg, bdata, num_trees, &gen, /*num_threads=*/ num_threads);

  return std::make_unique<py_forest<ft_type>>(std::move(forest));
}

std::unique_ptr<py_forest<ft_type>> load_forest(const std::string& data) {
  std::string_view vdata(data);
  std::unique_ptr<forest<ft_type>> forest = fast_tree::forest<ft_type>::load(&vdata);

  return std::make_unique<py_forest<ft_type>>(std::move(forest));
}

std::unique_ptr<py_forest<ft_type>> load_forest_from_file(const std::string& path) {
  mapfile mf(path.c_str());
  std::string_view vdata(mf);
  std::unique_ptr<forest<ft_type>> forest = fast_tree::forest<ft_type>::load(&vdata);

  return std::make_unique<py_forest<ft_type>>(std::move(forest));
}

}
}

PYBIND11_MODULE(fast_tree_pylib, mod) {
  using forest_type = fast_tree::pymod::py_forest<fast_tree::pymod::ft_type>;

  py::class_<forest_type>(mod, "Forest")
      .def("__len__", &forest_type::size)
      .def("str", &forest_type::str,
           py::arg("precision") = -1)
      .def("eval", &forest_type::eval,
           py::arg("data"));

  mod.def("create_forest",
          &fast_tree::pymod::create_forest,
          py::arg("columns"),
          py::arg("target"),
          py::arg("num_trees"),
          py::arg("opts") = py::dict(),
          py::arg("seed") = 17,
          py::arg("num_threads") = 0);

  mod.def("load_forest",
          &fast_tree::pymod::load_forest,
          py::arg("data"));
  mod.def("load_forest_from_file",
          &fast_tree::pymod::load_forest_from_file,
          py::arg("path"));
}
