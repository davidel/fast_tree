cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)

add_compile_options(
  "-O3"
  "-g"
)

include_directories(
  "/usr/local/include"
)

link_directories(
  "/usr/local/lib"
)

project(fast_tree)


file(GLOB FAST_TREE_HEADERS "include/fast_tree/*.h")
file(GLOB FAST_TREE_SOURCES "src/*.cc")

add_library(fast_tree STATIC
  ${FAST_TREE_HEADERS}
  ${FAST_TREE_SOURCES}
)

set_target_properties(fast_tree PROPERTIES
  LINKER_LANGUAGE CXX
)

target_include_directories(fast_tree PUBLIC
  "${PROJECT_SOURCE_DIR}/include"
)


find_package(Python3 COMPONENTS Development Interpreter)
find_package(Eigen3 CONFIG)
find_package(pybind11 CONFIG)

file(GLOB FAST_TREE_PYLIB_HEADERS "python/*.h")
file(GLOB FAST_TREE_PYLIB_SOURCES "python/*.cc")

pybind11_add_module(fast_tree_pylib MODULE
  ${FAST_TREE_PYLIB_HEADERS}
  ${FAST_TREE_PYLIB_SOURCES}
)

target_include_directories(fast_tree_pylib PUBLIC
  ${Python3_INCLUDE_DIRS}
  ${EIGEN3_INCLUDE_DIR}
)

target_link_directories(fast_tree_pylib PUBLIC
  ${Python3_LIB_DIRS}
)

set_target_properties(fast_tree_pylib PROPERTIES
  LINKER_LANGUAGE CXX
)

target_link_libraries(fast_tree_pylib PUBLIC
  fast_tree
  ${Python3_LIBRARIES}
)


add_subdirectory("test")
