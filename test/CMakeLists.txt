cmake_minimum_required(VERSION 3.10)

# Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24:
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
  cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)
FetchContent_Declare(
  GoogleTest
  URL https://github.com/google/googletest/archive/8ded48c37be09d8cc3665af1b414c5d53c0862e7.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(GoogleTest)

enable_testing()

project(fast_tree_test)

add_executable(fast_tree_test
  "test.cc"
)

set_target_properties(fast_tree_test PROPERTIES
  LINKER_LANGUAGE CXX
)

target_link_libraries(fast_tree_test PUBLIC
  fast_tree
  dcpl
  gtest_main
  pthread
)

include(GoogleTest)
gtest_discover_tests(fast_tree_test)
