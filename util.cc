#include "fast_tree/util.h"

namespace fast_tree {

bitmap create_bitmap(size_t size, span<const size_t> indices) {
  bitmap bmap(size, false);

  for (size_t ix : indices) {
    bmap[ix] = true;
  }

  return bmap;
}

fvector<size_t> reduce_indices(span<const size_t> indices, const bitmap& bmap,
                               std::pmr::memory_resource* mem) {
  fvector<size_t> rindices(mem != nullptr ? mem : std::pmr::get_default_resource());

  rindices.reserve(indices.size());
  for (size_t ix : indices) {
    if (bmap[ix]) {
      rindices.push_back(ix);
    }
  }

  return rindices;
}

fvector<size_t> iota(size_t size, size_t base, std::pmr::memory_resource* mem) {
  fvector<size_t> indices(size, base, mem != nullptr ? mem : std::pmr::get_default_resource());

  std::iota(indices.begin(), indices.end(), base);

  return indices;
}

}
