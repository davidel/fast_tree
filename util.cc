#include "fast_tree/util.h"

namespace fast_tree {

bitmap create_bitmap(size_t size, span<const size_t> indices) {
  bitmap bmap(size, false);

  for (size_t ix : indices) {
    bmap[ix] = true;
  }

  return bmap;
}

std::vector<size_t> reduce_indices(span<const size_t> indices, const bitmap& bmap) {
  std::vector<size_t> rindices;

  rindices.reserve(indices.size());
  for (size_t ix : indices) {
    if (bmap[ix]) {
      rindices.push_back(ix);
    }
  }

  return rindices;
}

std::vector<size_t> iota(size_t size, size_t base) {
  std::vector<size_t> indices(size);

  std::iota(indices.begin(), indices.end(), base);

  return indices;
}

std::string_view read_line(std::string_view* data) {
  std::string_view::size_type pos = data->find_first_of('\n');
  std::string_view ln;

  if (pos == std::string_view::npos) {
    ln = *data;
    *data = std::string_view();
  } else {
    ln = std::string_view(data->data(), pos);
    data->remove_prefix(pos + 1);
  }

  return ln;
}

}
