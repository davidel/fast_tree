cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)

# We need this since we are making fast_tree a static library.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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

include(FetchContent)

FetchContent_Declare(dcpl
  GIT_REPOSITORY https://github.com/davidel/dcpl.git
  GIT_TAG main
)
FetchContent_MakeAvailable(dcpl)


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
  dcpl
  ${Python3_LIBRARIES}
)

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  if (NOT FAST_TREE_DISABLE_TESTING)
    add_subdirectory("test")
  endif()
endif()

