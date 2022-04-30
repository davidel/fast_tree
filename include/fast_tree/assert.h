#pragma once

#include <sstream>
#include <stdexcept>

namespace fast_tree {
namespace detail {

struct base_asserter { };

struct asserter : public base_asserter {
  explicit asserter(const char* msg) {
    error_stream << "Assert failed: " << msg;
  }

  ~asserter() noexcept(false) {
    throw std::runtime_error(error_stream.str());
  }

  template <typename T>
  asserter& operator<<(const T& value) {
    if (!streamed) {
      error_stream << "; ";
      streamed = true;
    }
    error_stream << value;
    return *this;
  }

  std::stringstream error_stream;
  bool streamed = false;
};

}

#define FT_ASSERT(cond) (cond) ? detail::base_asserter() : detail::asserter(#cond)

}
