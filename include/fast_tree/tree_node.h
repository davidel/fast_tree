#pragma once

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "dcpl/assert.h"
#include "dcpl/constants.h"
#include "dcpl/types.h"
#include "dcpl/utils.h"

namespace fast_tree {

template <typename T>
class tree_node {
  static constexpr std::string_view tree_begin = std::string_view("TREE BEGIN");
  static constexpr std::string_view tree_end = std::string_view("TREE END");
  static constexpr dcpl::int_t invalid_id = -1;

 public:
  using value_type = T;

  explicit tree_node(std::vector<T> values) :
      splitter_(),
      values_(std::move(values)) {
  }

  tree_node(std::size_t index, const T& splitter) :
      index_(index),
      splitter_(splitter) {
  }

  tree_node(const tree_node&) = delete;

  tree_node(tree_node&&) = delete;

  tree_node& operator=(const tree_node&) = delete;

  tree_node& operator=(tree_node&&) = delete;

  bool is_leaf() const {
    return index_ == dcpl::consts::invalid_index;
  }

  std::size_t index() const {
    return index_;
  }

  T splitter() const {
    return splitter_;
  }

  std::span<const T> values() const {
    return values_;
  }

  const tree_node* left() const {
    return left_.get();
  }

  const tree_node* right() const {
    return right_.get();
  }

  void set_left(std::unique_ptr<tree_node> node) {
    left_ = std::move(node);
  }

  void set_right(std::unique_ptr<tree_node> node) {
    right_ = std::move(node);
  }

  std::span<const T> eval(std::span<const T> row) const {
    const tree_node* node = this;

    while (!node->is_leaf()) {
      T row_value = row[node->index()];

      if (row_value < node->splitter()) {
        node = node->left();
      } else {
        node = node->right();
      }
    }

    return node->values_;
  }

  void store(std::ostream* stream, int precision = -1) const {
    struct entry {
      explicit entry(const tree_node* node) :
          node(node) {
      }

      const tree_node* node = nullptr;
      std::size_t left_idx = dcpl::consts::invalid_index;
      std::size_t right_idx = dcpl::consts::invalid_index;
    };

    std::vector<std::unique_ptr<entry>> stack;

    stack.emplace_back(std::make_unique<entry>(this));
    for (std::size_t rindex = 0; stack.size() > rindex; ++rindex) {
      entry* ent = stack[rindex].get();

      if (!ent->node->is_leaf()) {
        ent->left_idx = stack.size();
        stack.emplace_back(std::make_unique<entry>(ent->node->left()));

        ent->right_idx = stack.size();
        stack.emplace_back(std::make_unique<entry>(ent->node->right()));
      }
    }

    if (precision >= 0) {
      (*stream) << std::setprecision(precision);
    }

    (*stream) << tree_begin << "\n";
    for (std::size_t i = stack.size(); i > 0; --i) {
      const entry& ent = *stack[i - 1];

      (*stream) << (i - 1);
      if (ent.node->is_leaf()) {
        DCPL_ASSERT(ent.left_idx == dcpl::consts::invalid_index) << ent.left_idx;
        DCPL_ASSERT(ent.right_idx == dcpl::consts::invalid_index) << ent.right_idx;

        (*stream) << " " << invalid_id << " " << invalid_id;
        for (T value : ent.node->values()) {
          (*stream) << " " << value;
        }
      } else {
        DCPL_ASSERT(ent.left_idx != dcpl::consts::invalid_index);
        DCPL_ASSERT(ent.right_idx != dcpl::consts::invalid_index);

        (*stream) << " " << ent.left_idx << " " << ent.right_idx
                  << " " << ent.node->index() << " " << ent.node->splitter();
      }
      (*stream) << "\n";
    }
    (*stream) << tree_end << "\n";
  }

  static std::unique_ptr<tree_node> load(std::string_view* data) {
    std::string_view remaining = *data;
    std::string_view ln = dcpl::read_line(&remaining);

    DCPL_ASSERT(ln == tree_begin) << "Invalid tree open statement: " << ln;

    std::map<dcpl::int_t, std::unique_ptr<tree_node>> nodes;
    dcpl::int_t root_id = invalid_id;

    while (!remaining.empty()) {
      ln = dcpl::read_line(&remaining);
      if (ln == tree_end) {
        break;
      }

      std::string_view wln = ln;
      dcpl::int_t id = get_next_value<dcpl::int_t>(&wln);
      dcpl::int_t left_idx = get_next_value<dcpl::int_t>(&wln);
      dcpl::int_t right_idx = get_next_value<dcpl::int_t>(&wln);

      if (left_idx == invalid_id) {
        DCPL_ASSERT(right_idx == invalid_id)
            << "Node should be leaf while right index is " << right_idx;

        std::vector<T> values;

        for (;;) {
          std::optional<T> value = next_value<T>(&wln);

          if (!value) {
            break;
          }
          values.push_back(*value);
        }

        nodes[id] = std::make_unique<tree_node>(std::move(values));
      } else {
        dcpl::int_t idx = get_next_value<dcpl::int_t>(&wln);
        T split_value = get_next_value<T>(&wln);

        std::unique_ptr<tree_node> node = std::make_unique<tree_node>(idx, split_value);

        auto lit = nodes.find(left_idx);
        DCPL_ASSERT(lit != nodes.end())
            << "Missing left index node " << left_idx << " for " << id;

        auto rit = nodes.find(right_idx);
        DCPL_ASSERT(rit != nodes.end())
            << "Missing right index node " << right_idx << " for " << id;

        node->set_left(std::move(lit->second));
        node->set_right(std::move(rit->second));
        nodes[id] = std::move(node);
      }
      root_id = id;
    }
    DCPL_ASSERT(ln == tree_end)
        << "Unbale to find tree end statement (\"" << tree_end << "\")";

    std::unique_ptr<tree_node> root;

    if (root_id != invalid_id) {
      auto rit = nodes.find(root_id);
      DCPL_ASSERT(rit != nodes.end()) << "Missing root index node " << root_id;

      root = std::move(rit->second);
    }
    for (auto& it : nodes) {
      DCPL_ASSERT(it.second == nullptr) << "Stray node left on stack for id " << it.first;
    }
    *data = remaining;

    return root;
  }

 private:
  template <typename U>
  static std::optional<U> next_value(std::string_view* ln) {
    std::string_view::size_type pos = ln->find_first_not_of(' ');

    if (pos == std::string_view::npos) {
      return std::nullopt;
    }
    if (pos > 0) {
      ln->remove_prefix(pos);
    }

    std::string_view::size_type vpos = ln->find_first_of(' ');
    std::string_view vdata;

    if (vpos != std::string_view::npos) {
      vdata = std::string_view(ln->data(), vpos);
      ln->remove_prefix(vpos + 1);
    } else {
      vdata = *ln;
      *ln = std::string_view();
    }

    return dcpl::from_chars<U>(vdata);
  }

  template <typename U>
  static U get_next_value(std::string_view* ln) {
    std::optional<U> value = next_value<U>(ln);

    DCPL_ASSERT(value) << "Required value missing";

    return *value;
  }

  std::size_t index_ = dcpl::consts::invalid_index;
  T splitter_;
  std::vector<T> values_;
  std::unique_ptr<tree_node> left_;
  std::unique_ptr<tree_node> right_;
};

}
