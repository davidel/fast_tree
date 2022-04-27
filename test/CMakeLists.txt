cmake_minimum_required(VERSION 3.10)

project(fast_tree_test)

add_executable(fast_tree_test
  "test.cc"
)

set_target_properties(fast_tree_test PROPERTIES
  LINKER_LANGUAGE CXX
)

target_link_libraries(fast_tree_test
  PUBLIC fast_tree
  gtest
  pthread
)
