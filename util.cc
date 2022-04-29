#include "fast_tree/util.h"

namespace fast_tree {

bitmap create_bitmap(size_t size, span<const size_t> indices) {
  bitmap bmap(size, false);

  for (size_t ix : indices) {
    bmap[ix] = true;
  }

  return bmap;
}

fvector<size_t> reduce_indices(span<const size_t> indices, const bitmap& bmap) {
  fvector<size_t> rindices;

  rindices.reserve(indices.size());
  for (size_t ix : indices) {
    if (bmap[ix]) {
      rindices.push_back(ix);
    }
  }

  return rindices;
}

fvector<size_t> iota(size_t size, size_t base) {
  fvector<size_t> indices(size);

  std::iota(indices.begin(), indices.end(), base);

  return indices;
}

}
