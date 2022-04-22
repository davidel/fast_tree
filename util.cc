#include "fast_tree/util.h"

namespace fast_tree {

std::vector<size_t> reduce_indices(span<const size_t> indices, const bitmap& bmap) {
  std::vector<size_t> rindices;

  rindices.reserve(indices.size() / 2);
  for (auto idx : indices) {
    if (bmap[idx]) {
      rindices.push_back(idx);
    }
  }

  return rindices;
}

}
