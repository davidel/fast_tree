#include "gtest/gtest.h"

#include <string>
#include <vector>

#include "fast_tree/span.h"
#include "fast_tree/string_formatter.h"
#include "fast_tree/util.h"

namespace fast_tree_test {

TEST(StringFormatter, API) {
  fast_tree::string_formatter sf;

  sf << "This " << 1 << " is a test for " << 2.3;
  EXPECT_EQ(sf.str(), std::string("This 1 is a test for 2.3"));
}

TEST(SpanTest, API) {
  int array[] = {1, 2, 3, 4, 5, 6, 7, 8};
  fast_tree::span<int> sp_arr(array);

  EXPECT_EQ(sp_arr.size(), 8);
  EXPECT_EQ(sp_arr[1], 2);
  EXPECT_EQ(sp_arr.at(2), 3);

  fast_tree::span<int> ssp_arr = sp_arr.subspan(2, 4);

  EXPECT_EQ(ssp_arr.size(), 4);
  EXPECT_EQ(ssp_arr[2], 5);
  EXPECT_EQ(ssp_arr.at(3), 6);
}

TEST(UtilTest, Argsort) {
  float array[] = {1.2, -0.8, 12.44, 8.9, 5.1, 16.25, 2.4};
  fast_tree::span<float> sp_arr(array);

  std::vector<size_t> indices = fast_tree::argsort(sp_arr);
  for (size_t i = 1; i < indices.size(); ++i) {
    EXPECT_LE(sp_arr[indices[i - 1]], sp_arr[indices[i]]);
  }
}

}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
