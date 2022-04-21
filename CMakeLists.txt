cmake_minimum_required(VERSION 3.10)

add_compile_options(
  "-std=c++17"
)

include_directories(
  "/usr/local/include"
)

link_directories(
  "/usr/local/lib"
)

project(fast_tree)

add_library(fast_tree STATIC
  "include/fast_tree/span.h"
  "include/fast_tree/string_formatter.h"

  "util.cc"
)

set_target_properties(fast_tree PROPERTIES
  LINKER_LANGUAGE CXX
)

target_include_directories(fast_tree PUBLIC
  "${PROJECT_SOURCE_DIR}/include"
)

add_subdirectory("test")
