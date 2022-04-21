#include "gtest/gtest.h"

#include <string>
#include <vector>

#include "fast_tree/span.h"
#include "fast_tree/string_formatter.h"

namespace fast_tree_test {

TEST(SpanTest, API) {
  int array[] = {1, 2, 3, 4, 5, 6, 7, 8};
  fast_tree::span<int> sp_arr(array);

  EXPECT_EQ(sp_arr.size(), 8);
  EXPECT_EQ(sp_arr[1], 2);

  fast_tree::span<int> ssp_arr = sp_arr.subspan(2, 4);

  EXPECT_EQ(ssp_arr.size(), 4);
  EXPECT_EQ(ssp_arr[2], 5);
}

}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
