#include <random>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "fast_tree/build_data.h"
#include "fast_tree/data.h"
#include "fast_tree/span.h"
#include "fast_tree/storage_span.h"
#include "fast_tree/string_formatter.h"
#include "fast_tree/tree_node.h"
#include "fast_tree/types.h"
#include "fast_tree/util.h"

namespace fast_tree_test {

TEST(StringFormatter, API) {
  fast_tree::string_formatter sf;

  sf << "This " << 1 << " is a test for " << 2.3;
  EXPECT_EQ(sf.str(), std::string("This 1 is a test for 2.3"));
}

TEST(SpanTest, API) {
  int array[] = {1, 2, 3, 4, 5, 6, 7, 8};
  fast_tree::span<const int> sp_arr(array);

  EXPECT_EQ(sp_arr.size(), 8);
  EXPECT_EQ(sp_arr[1], 2);
  EXPECT_EQ(sp_arr.at(2), 3);

  fast_tree::span<const int> ssp_arr = sp_arr.subspan(2, 4);

  EXPECT_EQ(ssp_arr.size(), 4);
  EXPECT_EQ(ssp_arr[2], 5);
  EXPECT_EQ(ssp_arr.at(3), 6);

  fast_tree::span<int> wsp_arr(array);

  EXPECT_EQ(wsp_arr.size(), 8);
  wsp_arr[2] = -1;
  EXPECT_EQ(array[2], -1);
}

TEST(StorageSpanTest, API) {
  int array[] = {1, 2, 3, 4, 5, 6, 7, 8};
  fast_tree::storage_span<int> ssp_arr(array);

  EXPECT_EQ(ssp_arr.size(), 8);
  EXPECT_EQ(ssp_arr.data().data(), array);

  fast_tree::storage_span<int>
      vsp_arr(std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8});
  EXPECT_EQ(vsp_arr.size(), 8);
  EXPECT_NE(vsp_arr.storage(), nullptr);

  fast_tree::storage_span<int> empty;
  empty = vsp_arr;
  EXPECT_EQ(empty.size(), 8);

  empty = ssp_arr;
  EXPECT_EQ(empty.size(), 8);
  EXPECT_EQ(empty.data().data(), array);
}

TEST(UtilTest, Argsort) {
  float array[] = {1.2, -0.8, 12.44, 8.9, 5.1, 16.25, 2.4};
  fast_tree::span<float> sp_arr(array);

  std::vector<size_t> indices = fast_tree::argsort(sp_arr);
  for (size_t i = 1; i < indices.size(); ++i) {
    EXPECT_LE(sp_arr[indices[i - 1]], sp_arr[indices[i]]);
  }
}

TEST(UtilTest, ToVector) {
  float array[] = {1.2, -0.8, 12.44, 8.9, 5.1, 16.25, 2.4};
  fast_tree::span<const float> sp_arr(array);

  std::vector<float> vec = fast_tree::to_vector(sp_arr);

  ASSERT_EQ(vec.size(), sp_arr.size());
  for (size_t i = 0; i < vec.size(); ++i) {
    EXPECT_EQ(vec[i], sp_arr[i]);
  }
}

TEST(UtilTest, ReduceIndices) {
  size_t indices[] = {3, 1, 5, 2, 0, 4};
  fast_tree::bitmap bmap(10, false);

  bmap[1] = true;
  bmap[3] = true;
  bmap[0] = true;
  bmap[8] = true;

  std::vector<size_t> rindices = fast_tree::reduce_indices(indices, bmap);

  EXPECT_EQ(rindices.size(), 3);
  EXPECT_EQ(rindices[0], 3);
  EXPECT_EQ(rindices[1], 1);
  EXPECT_EQ(rindices[2], 0);
}

TEST(UtilTest, Resample) {
  std::mt19937_64 gen;
  std::vector<size_t> indices = fast_tree::resample(100, 90, &gen);

  EXPECT_LE(indices.size(), 90);
}

TEST(TreeNodeTest, API) {
  std::vector<float> values{1.2, 9.7, 0.3, 5.8};
  fast_tree::tree_node<float> leaf_node(values);

  EXPECT_TRUE(leaf_node.is_leaf());
  EXPECT_EQ(leaf_node.values().size(), values.size());

  fast_tree::tree_node<float> split_node(2, 3.14f);

  split_node.set_left(std::make_unique<fast_tree::tree_node<float>>(values));
  split_node.set_right(std::make_unique<fast_tree::tree_node<float>>(values));

  EXPECT_FALSE(split_node.is_leaf());
  EXPECT_EQ(split_node.index(), 2);
  EXPECT_EQ(split_node.splitter(), 3.14f);
}

TEST(DataTest, API) {
  std::vector<float> values{1.2f, 9.7f, 0.3f, 5.8f, -1.8f};
  fast_tree::span<const float> sp_values(values);

  fast_tree::real_data<float> rdata(sp_values);

  rdata.add_column(sp_values);
  rdata.add_column(sp_values);

  EXPECT_EQ(rdata.num_columns(), 2);
  EXPECT_EQ(rdata.num_rows(), 5);

  fast_tree::real_data<float>::cdata col = rdata.column(1);
  EXPECT_EQ(col.data().data(), sp_values.data());

  std::size_t indices[] = {1, 3, 4};
  std::vector<float> scol = rdata.column_sample(0, indices);
  EXPECT_EQ(scol.size(), 3);
  EXPECT_EQ(scol[1], 5.8f);

  std::mt19937_64 gen;
  std::unique_ptr<fast_tree::data<float>> sdata = rdata.resample(3, &gen);
  EXPECT_LE(sdata->num_rows(), 3);
  EXPECT_LE(sdata->target().size(), 3);
}

TEST(BuildDataTest, API) {
  static const size_t N = 20;
  std::mt19937_64 gen;
  fast_tree::real_data<float> rdata(fast_tree::randn<float>(N, &gen));

  for (size_t i = 0; i < 10; ++i) {
    rdata.add_column(fast_tree::randn<float>(N, &gen));
  }

  fast_tree::build_data<float> bdata(rdata);
  EXPECT_EQ(bdata.column(2).size(), N);
  EXPECT_EQ(bdata.column_indices(1).size(), N);

  fast_tree::build_data<float> sbdata(bdata, fast_tree::arange<size_t>(1, N, 2));
  EXPECT_EQ(sbdata.column(5).size(), 10);

  std::vector<size_t> inv_indices = sbdata.invmap_indices(
      6, fast_tree::arange<size_t>(0, 5));
  EXPECT_EQ(inv_indices.size(), 5);

  fast_tree::span<const size_t> cidx = sbdata.column_indices(6);
  EXPECT_LE(inv_indices.size(), cidx.size());
  for (size_t i = 0; i < inv_indices.size(); ++i) {
    EXPECT_EQ(inv_indices[i], cidx[i]);
  }
}

}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
