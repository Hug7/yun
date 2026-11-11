/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <memory>
#include <utility>
#include <vector>

#include "bdm_dist_matrix.h"
#include "bdm_location.h"
#include "pdm_node.h"
#include "prob_pattern.h"

namespace {

/**
 * @brief 构造一段 node 链, 便于按 [type, loc] 顺序搭出 anchor/successor
 */
class NodeChain {
 public:
  NodeChain() = default;

  Node* append(const ActivityType type, const Location* loc) {
    auto node = std::make_unique<Node>(type, loc);
    Node* cur = node.get();
    if (this->head_ == nullptr) {
      this->head_ = std::move(node);
    } else {
      cur->prev = this->tail_;
      this->tail_->next = std::move(node);
    }
    this->tail_ = cur;
    return cur;
  }

  Node* head() const { return this->head_.get(); }

 private:
  Node::UPtr head_;
  Node* tail_{nullptr};
};

/**
 * @brief 测试用站点池: 同一 ind 复用同一个 Location 对象
 * @details 生产代码用指针相等判断"同站点"(scenario 内 Location 唯一), 这里必须保持同样的身份语义
 */
class LocationPool {
 public:
  const Location* get(const int ind) {
    if (static_cast<std::size_t>(ind) >= this->locs_.size()) {
      this->locs_.resize(static_cast<std::size_t>(ind) + 1);
    }
    if (this->locs_[static_cast<std::size_t>(ind)] == nullptr) {
      this->locs_[static_cast<std::size_t>(ind)] = std::make_unique<Location>(
          "L" + std::to_string(ind), "", ind, 0, 0, nullptr, nullptr, nullptr);
    }
    return this->locs_[static_cast<std::size_t>(ind)].get();
  }

 private:
  std::vector<std::unique_ptr<Location>> locs_;
};

/**
 * @brief 测试用距离矩阵: d(i, j) = i * 10 + j, 对角为 0
 * @details 故意取非对称, 顺序写反时断言会失败
 */
std::unique_ptr<DistMatrix> BuildDistMatrix() {
  auto dist_matrix = std::make_unique<DistMatrix>(0, 10);
  for (int i = 0; i < 10; ++i) {
    for (int j = 0; j < 10; ++j) {
      dist_matrix->set_dist_time(i, j, i == j ? 0 : i * 10 + j, 0);
    }
  }
  return dist_matrix;
}

constexpr std::int64_t d(const int from, const int to) { return from == to ? 0 : from * 10 + to; }

}  // namespace

TEST(PatternSpmdEstimateDropDeltaDist, AppendsAfterAnchorAndBeforeSuccessor) {
  const auto dist_matrix = BuildDistMatrix();
  LocationPool locs;
  NodeChain chain;
  chain.append(ActivityType::START, locs.get(0));
  Node* anchor = chain.append(ActivityType::DROP, locs.get(3));
  Node* successor = chain.append(ActivityType::END, locs.get(4));

  const auto delta =
      PatternSPMD::estimate_drop_delta_dist(*dist_matrix, anchor, locs.get(5), locs.get(6));

  // 只算两端连接边: 块内部 d(5, 6) 不累计
  EXPECT_EQ(delta, d(3, 5) + d(6, 4) - d(3, 4));
  // 纯估算: 不改动链结构
  EXPECT_EQ(anchor->next.get(), successor);
  EXPECT_EQ(successor->prev, anchor);
}

TEST(PatternSpmdEstimateDropDeltaDist, SingleDropAtAnchorLocationCostsNothing) {
  const auto dist_matrix = BuildDistMatrix();
  LocationPool locs;
  NodeChain chain;
  chain.append(ActivityType::START, locs.get(0));
  Node* anchor = chain.append(ActivityType::DROP, locs.get(2));
  chain.append(ActivityType::END, locs.get(4));

  const auto delta =
      PatternSPMD::estimate_drop_delta_dist(*dist_matrix, anchor, locs.get(2), locs.get(2));

  EXPECT_EQ(delta, 0);
}

TEST(PatternSpmdEstimateDropDeltaDist, UsesFirstAndLastDropLocationsOnly) {
  const auto dist_matrix = BuildDistMatrix();
  LocationPool locs;
  NodeChain chain;
  chain.append(ActivityType::START, locs.get(0));
  Node* anchor = chain.append(ActivityType::DROP, locs.get(2));
  chain.append(ActivityType::DROP, locs.get(3));
  chain.append(ActivityType::END, locs.get(4));

  // 首 drop(3) 恰好等于 successor, 但该启发式不模拟合并, 仍按首/尾站点计算
  const auto delta =
      PatternSPMD::estimate_drop_delta_dist(*dist_matrix, anchor, locs.get(3), locs.get(1));

  EXPECT_EQ(delta, d(2, 3) + d(1, 4) - d(2, 4));
}

TEST(PatternSpmdEstimateDropDeltaDist, RepeatedDropLocationsDoNotDoubleCount) {
  const auto dist_matrix = BuildDistMatrix();
  LocationPool locs;
  NodeChain chain;
  chain.append(ActivityType::START, locs.get(0));
  Node* anchor = chain.append(ActivityType::DROP, locs.get(3));
  chain.append(ActivityType::END, locs.get(4));

  const auto delta =
      PatternSPMD::estimate_drop_delta_dist(*dist_matrix, anchor, locs.get(5), locs.get(5));

  EXPECT_EQ(delta, d(3, 5) + d(5, 4) - d(3, 4));
}

TEST(PatternSpmdEstimateDropDeltaDist, InsertsBeforeExistingDropNode) {
  const auto dist_matrix = BuildDistMatrix();
  LocationPool locs;
  NodeChain chain;
  chain.append(ActivityType::START, locs.get(0));
  Node* anchor = chain.append(ActivityType::DROP, locs.get(2));
  chain.append(ActivityType::DROP, locs.get(3));
  chain.append(ActivityType::END, locs.get(4));

  const auto delta =
      PatternSPMD::estimate_drop_delta_dist(*dist_matrix, anchor, locs.get(5), locs.get(5));

  EXPECT_EQ(delta, d(2, 5) + d(5, 3) - d(2, 3));
}
