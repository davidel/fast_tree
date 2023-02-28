#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "dcpl/assert.h"
#include "dcpl/mapfile.h"
#include "dcpl/py_utils.h"
#include "dcpl/storage_span.h"
#include "dcpl/string_formatter.h"
#include "dcpl/types.h"
#include "dcpl/utils.h"

#include "fast_tree/build_config.h"
#include "fast_tree/build_tree.h"
#include "fast_tree/data.h"
#include "fast_tree/forest.h"
#include "fast_tree/tree_node.h"

namespace py = pybind11;

namespace fast_tree {
namespace pymod {

using ft_type = float;

using arr_type = py::array_t<ft_type, py::array::c_style | py::array::forcecast>;

template <typename T>
T get_partial(T size, const py::dict& opts, const char* name, T defval) {
  py::object opt_value = dcpl::get_object(opts, name);

  if (opt_value.is_none()) {
    return defval;
  }

  T value{};

  if (py::isinstance<py::int_>(opt_value)) {
    value = std::min<T>(size, opt_value.cast<dcpl::int_t>());
  } else if (py::isinstance<py::float_>(opt_value)) {
    value = std::min<T>(size, static_cast<T>(size * opt_value.cast<double>()));
  } else if (py::isinstance<py::str>(opt_value)) {
    std::string sval = opt_value.cast<std::string>();

    if (sval == "sqrt" || sval == "auto") {
      value = static_cast<T>(std::ceil(std::sqrt(size)));
    } else {
      throw std::invalid_argument(
          dcpl::_S() << "Invalid mode \"" << sval << "\" for \"" << name << "\" option");
    }
  } else {
    throw std::invalid_argument(
        dcpl::_S() << "Invalid type for \"" << name << "\" option (must be int or float)");
  }

  return value;
}

std::span<ft_type> array_span(const arr_type& arr) {
  DCPL_ASSERT(arr.ndim() == 1) << "Input has multi-dimensional shape: " <<
      std::span(arr.shape(), arr.ndim());

  // We const-cast but it is safe as the forest/tree API never writes into the buffers.
  return std::span<ft_type>(const_cast<ft_type*>(arr.data()), arr.size());
}

build_config get_build_config(std::size_t num_rows, std::size_t num_columns,
                              const py::dict& opts) {
  build_config bcfg;

  bcfg.num_rows = get_partial<std::size_t>(num_rows, opts, "max_rows", bcfg.num_rows);
  bcfg.num_columns = get_partial<std::size_t>(num_columns, opts, "max_columns", bcfg.num_columns);
  bcfg.min_leaf_size = dcpl::get_value_or<std::size_t>(opts, "min_leaf_size", bcfg.min_leaf_size);
  bcfg.max_depth = dcpl::get_value_or<std::size_t>(opts, "max_depth", bcfg.max_depth);
  bcfg.num_split_points = dcpl::get_value_or<std::size_t>(opts, "num_split_points", bcfg.num_split_points);
  bcfg.min_split_error = dcpl::get_value_or<double>(opts, "min_split_error", bcfg.min_split_error);
  bcfg.same_eps = dcpl::get_value_or<double>(opts, "same_eps", bcfg.same_eps);

  return bcfg;
}

template <typename T>
struct py_forest {
  explicit py_forest(std::unique_ptr<forest<T>> forest_ptr) :
      forest_ptr(std::move(forest_ptr)) {
  }

  std::size_t size() const {
    return forest_ptr->size();
  }

  std::string dumps(int precision) const {
    std::stringstream ss;

    forest_ptr->store(&ss, /*precision=*/ precision);

    return ss.str();
  }

  std::vector<arr_type> eval(const arr_type& data) const {
    std::size_t num_rows = data.shape(0);
    std::size_t num_columns = data.shape(1);
    std::vector<arr_type> result;

    result.reserve(num_rows);

    auto adata = data.unchecked<2>();
    std::vector<ft_type> row(num_columns);

    for (std::size_t i = 0; i < num_rows; ++i) {
      for (std::size_t j = 0; j < num_columns; ++j) {
        row[j] = adata(i, j);
      }

      std::vector<std::span<const ft_type>> rres = forest_ptr->eval(row);
      std::size_t rsize = 0;

      for (std::span<const ft_type>& s : rres) {
        rsize += s.size();
      }

      arr_type rarr(arr_type::ShapeContainer{rsize});
      auto ares = rarr.mutable_unchecked<1>();
      std::size_t x = 0;

      for (std::span<const ft_type>& s : rres) {
        for (ft_type v : s) {
          ares(x) = v;
          ++x;
        }
      }

      result.push_back(std::move(rarr));
    }

    return result;
  }

  std::unique_ptr<forest<T>> forest_ptr;
};

std::unique_ptr<py_forest<ft_type>> create_forest(
    const std::vector<arr_type>& columns, arr_type target, py::dict opts) {
  std::size_t num_trees = dcpl::get_value_or<std::size_t>(opts, "num_trees", 100);
  std::size_t seed = dcpl::get_value_or<std::size_t>(opts, "seed", 161862243);
  std::size_t num_threads = dcpl::get_value_or<std::size_t>(opts, "num_threads", 0);

  build_config bcfg = get_build_config(target.size(), columns.size(), opts);

  std::unique_ptr<data<ft_type>>
      rdata = std::make_unique<data<ft_type>>(array_span(target));

  for (auto& col : columns) {
    rdata->add_column(array_span(col));
  }

  std::shared_ptr<build_data<ft_type>>
      bdata = std::make_shared<build_data<ft_type>>(*rdata);
  dcpl::rnd_generator gen(seed);

  py::gil_scoped_release release;

  std::unique_ptr<forest<ft_type>>
      forest_ptr = build_forest(bcfg, bdata, num_trees, &gen, /*num_threads=*/ num_threads);

  return std::make_unique<py_forest<ft_type>>(std::move(forest_ptr));
}

std::unique_ptr<py_forest<ft_type>> load_forest(const std::string& data) {
  std::string_view vdata(data);
  std::unique_ptr<forest<ft_type>> forest_ptr = fast_tree::forest<ft_type>::load(&vdata);

  return std::make_unique<py_forest<ft_type>>(std::move(forest_ptr));
}

std::unique_ptr<py_forest<ft_type>> load_forest_from_file(const std::string& path) {
  dcpl::mapfile mf(path.c_str());
  std::string_view vdata(mf);
  std::unique_ptr<forest<ft_type>> forest_ptr = fast_tree::forest<ft_type>::load(&vdata);

  return std::make_unique<py_forest<ft_type>>(std::move(forest_ptr));
}

}
}

PYBIND11_MODULE(fast_tree_pylib, mod) {
  using forest_type = fast_tree::pymod::py_forest<fast_tree::pymod::ft_type>;

  py::class_<forest_type>(mod, "Forest")
      .def("__len__", &forest_type::size)
      .def("dumps", &forest_type::dumps,
           py::arg("precision") = -1)
      .def("eval", &forest_type::eval,
           py::arg("data"));

  mod.def("create_forest",
          &fast_tree::pymod::create_forest,
          py::arg("columns"),
          py::arg("target"),
          py::arg("opts") = py::dict());

  mod.def("load_forest",
          &fast_tree::pymod::load_forest,
          py::arg("data"));
  mod.def("load_forest_from_file",
          &fast_tree::pymod::load_forest_from_file,
          py::arg("path"));
}

