#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "fast_tree/mapfile.h"
#include "fast_tree/util.h"

namespace fast_tree {

mapfile::mapfile(const char* path) {
  fd_ = ::open(path, O_RDONLY);
  FT_ASSERT(fd_ != -1) << "Unable to open file: " << path;

  cleanup clean_file([fd = fd_]() { ::close(fd); });

  size_ = lseek(fd_, 0, SEEK_END);
  ::lseek(fd_, 0, SEEK_SET);

  base_ = ::mmap(nullptr, size_, PROT_READ, MAP_SHARED, fd_, 0);
  FT_ASSERT(base_ != MAP_FAILED) << "Failed to mmap file: " << path;

  clean_file.reset();
}

mapfile::~mapfile() {
  if (base_ != nullptr) {
    ::munmap(base_, size_);
  }
  if (fd_ != -1) {
    ::close(fd_);
  }
}

}
