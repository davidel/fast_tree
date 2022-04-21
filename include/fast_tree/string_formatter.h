#pragma once

#include <sstream>
#include <string>

namespace fast_tree {

class string_formatter {
public:
  template <typename T>
  string_formatter& operator<<(const T& value) {
    stream_ << value;
    return *this;
  }

  std::string str() const {
    return stream_.str();
  }

  operator std::string() const {
    return str();
  }

private:
    std::stringstream stream_;
};

}
