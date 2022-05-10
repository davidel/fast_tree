#include <sys/mman.h>
#include <sys/types.h>

#include <string_view>

#pragma once

namespace fast_tree {

class mapfile {
 public:
  explicit mapfile(const char* path);

  virtual ~mapfile();

  operator std::string_view() const {
    return std::string_view(reinterpret_cast<const char*>(base_), size_);
  }

 private:
  int fd_ = -1;
  void* base_ = MAP_FAILED;
  off_t size_ = 0;
};

}
