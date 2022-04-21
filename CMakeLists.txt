cmake_minimum_required(VERSION 3.10)

project(fast_tree)

add_library(fast_tree STATIC
  include/fast_tree/span.h
  include/fast_tree/string_formatter.h
)

set_target_properties(fast_tree PROPERTIES
  LINKER_LANGUAGE CXX
)
