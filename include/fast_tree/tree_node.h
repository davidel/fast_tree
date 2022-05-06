#pragma once

#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

#include "fast_tree/assert.h"
#include "fast_tree/constants.h"
#include "fast_tree/span.h"
#include "fast_tree/types.h"

namespace fast_tree {

template <typename T>
class tree_node {
  static constexpr std::string_view tree_begin = std::string_view("TREE BEGIN");
  static constexpr std::string_view tree_end = std::string_view("TREE END");

  using id_type = std::ptrdiff_t;

 public:
  using value_type = T;

  explicit tree_node(std::vector<T> values) :
      splitter_(),
      values_(std::move(values)) {
  }

  tree_node(size_t index, const T& splitter) :
      index_(index),
      splitter_(splitter) {
  }

  tree_node(const tree_node&) = delete;

  tree_node(tree_node&&) = delete;

  tree_node& operator=(const tree_node&) = delete;

  tree_node& operator=(tree_node&&) = delete;

  bool is_leaf() const {
    return index_ == consts::invalid_index;
  }

  size_t index() const {
    return index_;
  }

  T splitter() const {
    return splitter_;
  }

  span<const T> values() const {
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

  span<const T> eval(span<const T> row) const {
    const tree_node* node = this;

    while (!node->is_leaf()) {
      T row_value = row.at(node->index());

      if (row_value < node->splitter()) {
        node = node->left();
      } else {
        node = node->right();
      }
    }

    return node->values_;
  }

  void store(std::ostream* stream) const {
    struct entry {
      entry(const tree_node* node, size_t parent_idx) :
          node(node),
          parent_idx(parent_idx) {
      }

      const tree_node* node = nullptr;
      size_t parent_idx = consts::invalid_index;
      size_t left_idx = consts::invalid_index;;
      size_t right_idx = consts::invalid_index;
    };

    std::vector<std::unique_ptr<entry>> stack;

    stack.emplace_back(std::make_unique<entry>(this, 0));
    for (size_t rindex = 0; stack.size() > rindex; ++rindex) {
      entry* ent = stack[rindex].get();

      if (!ent->node->is_leaf()) {
        stack[ent->parent_idx]->left_idx = stack.size();
        stack.emplace_back(std::make_unique<entry>(ent->node->left(), stack.size()));

        stack[ent->parent_idx]->right_idx = stack.size();
        stack.emplace_back(std::make_unique<entry>(ent->node->right(), stack.size()));
      }
    }

    (*stream) << tree_begin << "\n";
    for (size_t i = stack.size(); i > 0; --i) {
      const entry& ent = *stack[i - 1];

      (*stream) << (i - 1);
      if (ent.node->is_leaf()) {
        FT_ASSERT(ent.left_idx == consts::invalid_index) << ent.left_idx;
        FT_ASSERT(ent.right_idx == consts::invalid_index) << ent.right_idx;

        (*stream) << " -1 -1";
        for (T value : ent.node->values()) {
          (*stream) << " " << value;
        }
      } else {
        FT_ASSERT(ent.left_idx != consts::invalid_index);
        FT_ASSERT(ent.right_idx != consts::invalid_index);

        (*stream) << " " << ent.left_idx << " " << ent.right_idx
                  << " " << ent.node->index() << " " << ent.node->splitter();
      }
      (*stream) << "\n";
    }
    (*stream) << tree_end << "\n";
  }

  static std::unique_ptr<tree_node> load(std::string_view data) {
    std::string_view remaining = data;
    std::string_view ln = read_line(&remaining);

    FT_ASSERT(ln == tree_begin) << "Invalid tree open statement: " << ln;

    std::map<id_type, std::unique_ptr<tree_node>> nodes;
    id_type root_id = -1;

    while (!remaining.empty()) {
      ln = read_line(&remaining);
      if (ln == tree_end) {
        break;
      }

      std::string_view wln = ln;
      id_type id = get_next_value<id_type>(&wln);
      id_type left_idx = get_next_value<id_type>(&wln);
      id_type right_idx = get_next_value<id_type>(&wln);

      if (left_idx < 0) {
        FT_ASSERT(right_idx < 0)
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
        id_type idx = get_next_value<id_type>(&wln);
        T split_value = get_next_value<T>(&wln);

        std::unique_ptr<tree_node> node = std::make_unique<tree_node>(idx, split_value);

        auto lit = nodes.find(left_idx);
        FT_ASSERT(lit != nodes.end())
            << "Missing left index node " << left_idx << " for " << id;

        auto rit = nodes.find(right_idx);
        FT_ASSERT(rit != nodes.end())
            << "Missing right index node " << right_idx << " for " << id;

        node->set_left(std::move(lit->second));
        node->set_right(std::move(rit->second));
        nodes[id] = std::move(node);
      }
      root_id = id;
    }
    FT_ASSERT(ln == tree_end)
        << "Unbale to find tree end statement (\"" << tree_end << "\")";

    auto rit = nodes.find(root_id);
    FT_ASSERT(rit != nodes.end()) << "Missing root index node " << root_id;

    std::unique_ptr<tree_node> root = std::move(rit->second);

    for (auto& it : nodes) {
      FT_ASSERT(it.second == nullptr) << "Stray node left on stack for id " << it.first;
    }

    return root;
  }

 private:
  static std::string_view read_line(std::string_view* data) {
    std::string_view::size_type pos = data->find_first_of('\n');
    std::string_view ln;

    if (pos == std::string_view::npos) {
      ln = *data;
      *data = std::string_view();
    } else {
      ln = std::string_view(data->data(), pos);
      data->remove_prefix(pos + 1);
    }

    return ln;
  }

  static void copy_buffer(std::string_view vdata, char* buffer, size_t size) {
    FT_ASSERT(vdata.size() < size) << "Value string size too big: " << vdata.size();

    // std::memcpy() is slower than this code for small buffers (we are usually
    // copying an handfull of bytes here) as it does a bunch of check on entry
    // to figure out if it can use xmm copies.
    const char* dptr = vdata.data();
    const char* dend = dptr + vdata.size();
    char* ptr = buffer;

    while (dptr < dend) {
      *ptr++ = *dptr++;
    }
    *ptr = '\0';
  }

  template<typename U,
           typename std::enable_if<std::is_integral<U>::value>::type* = nullptr>
  static std::optional<U> from_chars(std::string_view vdata) {
    // std::from_chars() is not fully implemented in clang.
    char buffer[128];

    copy_buffer(vdata, buffer, sizeof(buffer));
    char* eob = buffer;
    auto value = std::strtoll(buffer, &eob, 10);

    if (eob == buffer) {
      return std::nullopt;
    }

    return static_cast<U>(value);
  }

  template<class U,
           typename std::enable_if<std::is_floating_point<U>::value>::type* = nullptr>
  static std::optional<U> from_chars(std::string_view vdata) {
    // std::from_chars() is not fully implemented in clang.
    char buffer[128];

    copy_buffer(vdata, buffer, sizeof(buffer));
    char* eob = buffer;
    auto value = std::strtod(buffer, &eob);

    if (eob == buffer) {
      return std::nullopt;
    }

    return static_cast<U>(value);
  }

  template <typename U>
  static std::optional<U> next_value(std::string_view* ln) {
    std::string_view::size_type pos = ln->find_first_not_of(' ');

    if (pos == std::string_view::npos) {
      return std::nullopt;
    }
    if (pos > 0) {
      ln->remove_prefix(pos);
      if (ln->empty()) {
        return std::nullopt;
      }
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

    return from_chars<U>(vdata);
  }

  template <typename U>
  static U get_next_value(std::string_view* ln) {
    std::optional<U> value = next_value<U>(ln);

    FT_ASSERT(value) << "Required value missing";

    return *value;
  }

  size_t index_ = consts::invalid_index;
  T splitter_;
  std::vector<T> values_;
  std::unique_ptr<tree_node> left_;
  std::unique_ptr<tree_node> right_;
};

}
