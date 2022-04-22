#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "fast_tree/data.h"
#include "fast_tree/span.h"
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

TEST(TreeNodeTest, API) {
  std::vector<float> values{1.2, 9.7, 0.3, 5.8};
  fast_tree::tree_node<float> leaf_node(values);

  EXPECT_TRUE(leaf_node.is_leaf());

  fast_tree::tree_node<float> split_node(2, 3.14f);

  split_node.set_left(std::make_unique<fast_tree::tree_node<float>>(values));
  split_node.set_right(std::make_unique<fast_tree::tree_node<float>>(values));

  EXPECT_FALSE(split_node.is_leaf());
  EXPECT_EQ(split_node.index(), 2);
  EXPECT_EQ(split_node.splitter(), 3.14f);
}

TEST(DataTest, API) {
  std::vector<float> values{1.2, 9.7, 0.3, 5.8};
  fast_tree::span<const float> sp_values(values);

  fast_tree::real_data<float> rdata;

  rdata.add_column(sp_values);
  rdata.add_column(sp_values);

  EXPECT_EQ(rdata.num_columns(), 2);
  EXPECT_EQ(rdata.num_rows(), 4);

  fast_tree::real_data<float>::cdata col = rdata.column(1);
  EXPECT_EQ(col.data().data(), sp_values.data());
}

}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
