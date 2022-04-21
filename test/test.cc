#include "gtest/gtest.h"

#include <string>
#include <vector>

#include "fast_tree/span.h"
#include "fast_tree/string_formatter.h"

namespace fast_tree_test {

TEST(SpanTest, API) {
  int array[] = {1, 2, 3, 4};
  fast_tree::span<int> sp_arr(array);

  EXPECT_EQ(sp_arr.size(), 4);
}

}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
