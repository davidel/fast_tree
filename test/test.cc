#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <span>
#include <sstream>
#include <string>
#include <vector>

#include "dcpl/types.h"
#include "dcpl/utils.h"

#include "fast_tree/build_data.h"
#include "fast_tree/build_tree.h"
#include "fast_tree/build_tree_node.h"
#include "fast_tree/column_split.h"
#include "fast_tree/data.h"
#include "fast_tree/forest.h"
#include "fast_tree/tree_node.h"
#include "fast_tree/types.h"

#include "gtest/gtest.h"

namespace fast_tree_test {

template <typename T>
std::unique_ptr<fast_tree::data<T>> create_data(size_t nrows, size_t ncols) {
  dcpl::rnd_generator gen;
  std::unique_ptr<fast_tree::data<T>>
      rdata = std::make_unique<fast_tree::data<T>>(dcpl::randn<T>(nrows, &gen));

  for (size_t i = 0; i < ncols; ++i) {
    rdata->add_column(dcpl::randn<T>(nrows, &gen));
  }

  return rdata;
}

template <typename T>
std::unique_ptr<fast_tree::data<T>> create_circle_clusters(
    size_t nclusters, size_t cluster_size, T radius, T noise) {
  std::vector<T> target;
  double angle = 2.0 * M_PI / nclusters;

  for (size_t i = 0; i < nclusters; ++i) {
    for (size_t j = 0; j < cluster_size; ++j) {
      target.push_back(static_cast<T>(i * angle));
    }
  }

  dcpl::rnd_generator rgen;
  std::uniform_real_distribution<T> gen(-0.5, 0.5);

  std::unique_ptr<fast_tree::data<T>>
      rdata = std::make_unique<fast_tree::data<T>>(std::move(target));
  std::vector<T> x_data;
  std::vector<T> y_data;

  for (size_t i = 0; i < nclusters; ++i) {
    double cangle = static_cast<T>(i * angle);

    for (size_t j = 0; j < cluster_size; ++j) {
      double xangle = cangle + noise * gen(rgen);

      x_data.push_back(static_cast<T>(radius * std::cos(xangle)));
      y_data.push_back(static_cast<T>(radius * std::sin(xangle)));
    }
  }
  rdata->add_column(std::move(x_data));
  rdata->add_column(std::move(y_data));

  return rdata;
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
  std::span<const float> sp_values(values);

  fast_tree::data<const float> rdata(sp_values);

  rdata.add_column(sp_values);
  rdata.add_column(sp_values);

  EXPECT_EQ(rdata.num_columns(), 2);
  EXPECT_EQ(rdata.num_rows(), 5);

  fast_tree::data<const float>::cdata col = rdata.column(1);
  EXPECT_EQ(col.data().data(), sp_values.data());

  std::size_t indices[] = {1, 3, 4};
  std::vector<float> scol = rdata.column_sample(0, indices);
  EXPECT_EQ(scol.size(), 3);
  EXPECT_EQ(scol[1], 5.8f);
}

TEST(BuildDataTest, API) {
  static const size_t N = 20;
  static const size_t C = 10;
  std::unique_ptr<fast_tree::data<float>> rdata = create_data<float>(N, C);
  std::shared_ptr<fast_tree::build_data<float>>
      bdata = std::make_shared<fast_tree::build_data<float>>(*rdata);
  EXPECT_EQ(bdata->column(2).size(), N);

  std::shared_ptr<fast_tree::build_data<float>>
      sbdata = std::make_shared<fast_tree::build_data<float>>(
          bdata->data(), dcpl::arange<size_t>(1, N, 2));
  EXPECT_EQ(sbdata->indices().size(), C);
  EXPECT_EQ(sbdata->column(5).size(), C);
  EXPECT_EQ(sbdata->target().size(), C);

  size_t part_idx = bdata->partition_indices(4, 0.5);

  EXPECT_GT(part_idx, 0);
}

TEST(BuildTreeNodeTest, API) {
  static const size_t N = 100;
  static const size_t C = 10;
  std::unique_ptr<fast_tree::data<float>> rdata = create_data<float>(N, C);
  std::shared_ptr<fast_tree::build_data<float>>
      bdata = std::make_shared<fast_tree::build_data<float>>(*rdata);

  std::unique_ptr<fast_tree::tree_node<float>> root;

  auto setter = [&](std::unique_ptr<fast_tree::tree_node<float>> node) {
    root = std::move(node);
  };

  fast_tree::build_config bcfg;
  dcpl::rnd_generator gen;
  fast_tree::build_tree_node<float>::split_fn
      splitter = fast_tree::create_splitter<float>(bcfg, N, C, &gen);
  fast_tree::build_tree_node<float>
      btn(bcfg, std::move(bdata), std::move(setter), splitter, &gen);

  std::vector<std::unique_ptr<fast_tree::build_tree_node<float>>>
      split = btn.split();
  EXPECT_EQ(split.size(), 2);
}

TEST(BuildTreeTest, Tree) {
  static const size_t N = 100;
  static const size_t C = 10;
  std::unique_ptr<fast_tree::data<float>> rdata = create_data<float>(N, C);
  std::shared_ptr<fast_tree::build_data<float>>
      bdata = std::make_shared<fast_tree::build_data<float>>(*rdata);

  fast_tree::build_config bcfg;
  dcpl::rnd_generator gen;

  std::unique_ptr<fast_tree::tree_node<float>> root = fast_tree::build_tree(bcfg, bdata, &gen);
  ASSERT_NE(root, nullptr);
  EXPECT_FALSE(root->is_leaf());

  fast_tree::data<float>::cdata target = rdata->target();
  for (size_t r = 0; r < rdata->num_rows(); ++r) {
    std::vector<float> row = rdata->row(r);
    std::span<const float> evres = root->eval(row);
    EXPECT_GT(evres.size(), 0);
  }

  std::stringstream ss;

  root->store(&ss, /*precision=*/ 10);
  EXPECT_TRUE(!ss.str().empty());

  std::string svstr = ss.str();
  std::string_view svdata(svstr);
  std::unique_ptr<fast_tree::tree_node<float>>
      lroot = fast_tree::tree_node<float>::load(&svdata);

  EXPECT_TRUE(svdata.empty());

  for (size_t r = 0; r < rdata->num_rows(); ++r) {
    std::vector<float> row = rdata->row(r);
    std::span<const float> evres = root->eval(row);
    std::span<const float> levres = lroot->eval(row);
    EXPECT_EQ(evres, levres);
  }
}

TEST(BuildTreeTest, TreeAccuracy) {
  static const size_t N_CLUSTERS = 16;
  static const size_t CLUSTER_SIZE = 8;
  static const float RADIUS = 4.0f;
  static const float NOISE = 1e-2;

  std::unique_ptr<fast_tree::data<float>>
      rdata = create_circle_clusters<float>(N_CLUSTERS, CLUSTER_SIZE, RADIUS, NOISE);
  std::shared_ptr<fast_tree::build_data<float>>
      bdata = std::make_shared<fast_tree::build_data<float>>(*rdata);
  dcpl::rnd_generator gen;
  fast_tree::build_config bcfg;

  bcfg.min_leaf_size = 1;

  std::unique_ptr<fast_tree::tree_node<float>> root = fast_tree::build_tree(bcfg, bdata, &gen);

  fast_tree::data<float>::cdata target = rdata->target();
  for (size_t r = 0; r < rdata->num_rows(); ++r) {
    std::vector<float> row = rdata->row(r);
    std::span<const float> evres = root->eval(row);

    size_t matches = 0;

    for (float v : evres) {
      if (v == target[r]) {
        ++matches;
      }
    }
    EXPECT_EQ(matches, evres.size());
  }
}

TEST(BuildTreeTest, Forest) {
  static const size_t N = 240000;
  static const size_t C = 1000;
  static const size_t T = 4;
  std::unique_ptr<fast_tree::data<float>> rdata = create_data<float>(N, C);
  std::shared_ptr<fast_tree::build_data<float>>
      bdata = std::make_shared<fast_tree::build_data<float>>(*rdata);
  dcpl::rnd_generator gen;
  fast_tree::build_config bcfg;

  bcfg.num_rows = static_cast<size_t>(0.75 * N);
  bcfg.num_columns = static_cast<size_t>(std::sqrt(C));

  std::unique_ptr<fast_tree::forest<float>>
      forest = fast_tree::build_forest(bcfg, bdata, T, &gen);
  EXPECT_EQ(forest->size(), T);

  std::vector<float> row = rdata->row(N / 2);
  std::vector<std::span<const float>> results = forest->eval(row);
  EXPECT_EQ(results.size(), T);

  std::stringstream ss;

  forest->store(&ss, /*precision=*/ 10);
  EXPECT_TRUE(!ss.str().empty());

  std::string svstr = ss.str();
  std::string_view svdata(svstr);
  std::unique_ptr<fast_tree::forest<float>>
      lforest = fast_tree::forest<float>::load(&svdata);

  EXPECT_TRUE(svdata.empty());
  for (size_t r = 0; r < rdata->num_rows(); ++r) {
    std::vector<float> row = rdata->row(r);
    std::vector<std::span<const float>> evres = forest->eval(row);
    std::vector<std::span<const float>> levres = lforest->eval(row);
    EXPECT_EQ(evres, levres);
  }
}

}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
