/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pdm_node.h"

#include <gtest/gtest.h>

#include <memory>

#include "bdm_time_window_plan.h"
#include "pdm_activity.h"

namespace {

// 将 cur 挂到 prev_node 后面, 并修正 cur 的 prev
void LinkAfter(Node* prev_node, Node::UPtr cur) {
  Node* cur_ptr = cur.get();
  cur_ptr->prev = prev_node;
  prev_node->next = std::move(cur);
}

int ChainLen(const Node* head) {
  int len = 0;
  for (const Node* node = head; node != nullptr; node = node->next.get()) {
    ++len;
  }
  return len;
}

int ActivityChainLen(const Activity* first) {
  int len = 0;
  for (const Activity* activity = first; activity != nullptr; activity = activity->next.get()) {
    ++len;
  }
  return len;
}

// 构造 START -> PICK(2 activity) -> DROP(2 activity) -> END
// pick activity 的 related 指向配对的 drop activity, related_node 指向 drop 所在 node
Node::UPtr BuildChain() {
  auto start = std::make_unique<Node>(ActivityType::START, nullptr);
  auto pick_node = std::make_unique<Node>(ActivityType::PICK, nullptr);
  auto drop_node = std::make_unique<Node>(ActivityType::DROP, nullptr);
  auto end = std::make_unique<Node>(ActivityType::END, nullptr);

  auto pair_a = ActivityFactory::create_pair_activity(nullptr);
  auto pair_b = ActivityFactory::create_pair_activity(nullptr);
  pair_a.first->set_related_node(drop_node.get());
  pair_b.first->set_related_node(drop_node.get());

  pick_node->add_back_activity(std::move(pair_a.first));
  pick_node->add_back_activity(std::move(pair_b.first));
  drop_node->add_back_activity(std::move(pair_a.second));
  drop_node->add_back_activity(std::move(pair_b.second));

  pick_node->ptws.push_back(std::make_unique<TimeWindowPlan>(1, 2, 3, 4, 5, 6));
  pick_node->ptws.push_back(std::make_unique<TimeWindowPlan>(7, 8, 9, 10, 11, 12));
  pick_node->set_travel_dist(100);
  pick_node->set_travel_time(60);
  drop_node->set_travel_dist(200);
  drop_node->set_travel_time(120);
  end->set_travel_dist(300);
  end->set_travel_time(180);

  Node* start_ptr = start.get();
  Node* pick_ptr = pick_node.get();
  Node* drop_ptr = drop_node.get();
  LinkAfter(start_ptr, std::move(pick_node));
  LinkAfter(pick_ptr, std::move(drop_node));
  LinkAfter(drop_ptr, std::move(end));

  return start;
}

}  // namespace

TEST(NodeOpsDeepCopyChain, NullHeadReturnsNull) {
  EXPECT_EQ(NodeOps::deep_copy_chain(nullptr), nullptr);
}

TEST(NodeOpsDeepCopyChain, ClonesNodesAndActivityChains) {
  Node::UPtr src_head = BuildChain();

  Node::UPtr copy_head = NodeOps::deep_copy_chain(src_head.get());

  ASSERT_NE(copy_head, nullptr);
  EXPECT_EQ(ChainLen(copy_head.get()), 4);
  EXPECT_EQ(copy_head->prev, nullptr);

  const ActivityType expect_types[4] = {ActivityType::START, ActivityType::PICK, ActivityType::DROP,
                                        ActivityType::END};
  const long expect_dist[4] = {0, 100, 200, 300};
  const long expect_time[4] = {0, 60, 120, 180};

  const Node* src_node = src_head.get();
  Node* copy_node = copy_head.get();
  Node* copy_prev = nullptr;
  for (int ind = 0; copy_node != nullptr; ++ind) {
    EXPECT_EQ(copy_node->activity_type, expect_types[ind]);
    EXPECT_EQ(copy_node->travel_dist, expect_dist[ind]);
    EXPECT_EQ(copy_node->travel_time, expect_time[ind]);
    EXPECT_EQ(copy_node->loc, src_node->loc);
    // prev 指向新链的前驱, 而非源链
    EXPECT_EQ(copy_node->prev, copy_prev);
    EXPECT_NE(copy_node, src_node);

    // ptws 逐个深拷贝
    ASSERT_EQ(copy_node->ptws.size(), src_node->ptws.size());
    for (size_t u = 0; u < copy_node->ptws.size(); ++u) {
      EXPECT_NE(copy_node->ptws[u].get(), src_node->ptws[u].get());
      EXPECT_EQ(copy_node->ptws[u]->early_arr, src_node->ptws[u]->early_arr);
      EXPECT_EQ(copy_node->ptws[u]->late_arr, src_node->ptws[u]->late_arr);
      EXPECT_EQ(copy_node->ptws[u]->early_dest, src_node->ptws[u]->early_dest);
      EXPECT_EQ(copy_node->ptws[u]->late_dest, src_node->ptws[u]->late_dest);
      EXPECT_EQ(copy_node->ptws[u]->wait_time, src_node->ptws[u]->wait_time);
      EXPECT_EQ(copy_node->ptws[u]->over_time, src_node->ptws[u]->over_time);
    }

    // activity 子链: 数量、顺序、prev/last 一致, 对象本身是新的
    EXPECT_EQ(ActivityChainLen(copy_node->first.get()), ActivityChainLen(src_node->first.get()));
    const Activity* src_activity = src_node->first.get();
    Activity* copy_activity = copy_node->first.get();
    Activity* copy_activity_prev = nullptr;
    while (copy_activity != nullptr) {
      EXPECT_NE(copy_activity, src_activity);
      EXPECT_EQ(copy_activity->activity_type, src_activity->activity_type);
      EXPECT_EQ(copy_activity->order, src_activity->order);
      EXPECT_EQ(copy_activity->prev, copy_activity_prev);
      copy_activity_prev = copy_activity;
      copy_activity = copy_activity->next.get();
      src_activity = src_activity->next.get();
    }
    if (src_node->last != nullptr) {
      EXPECT_EQ(copy_node->last, copy_activity_prev);
      EXPECT_EQ(copy_node->last->next, nullptr);
    } else {
      EXPECT_EQ(copy_node->last, nullptr);
    }

    copy_prev = copy_node;
    copy_node = copy_node->next.get();
    src_node = src_node->next.get();
  }
  EXPECT_EQ(ChainLen(src_head.get()), 4);
}

TEST(NodeOpsDeepCopyChain, RemapsRelatedPointersIntoCopiedChain) {
  Node::UPtr src_head = BuildChain();
  Node* src_pick_node = src_head->next.get();
  Node* src_drop_node = src_pick_node->next.get();

  Node::UPtr copy_head = NodeOps::deep_copy_chain(src_head.get());

  Node* copy_pick_node = copy_head->next.get();
  Node* copy_drop_node = copy_pick_node->next.get();
  ASSERT_NE(copy_drop_node, nullptr);

  // related 指向新链里配对的 activity, 且互相指回
  EXPECT_EQ(copy_pick_node->first->related, copy_drop_node->first.get());
  EXPECT_EQ(copy_pick_node->last->related, copy_drop_node->last);
  EXPECT_EQ(copy_drop_node->first->related, copy_pick_node->first.get());
  EXPECT_EQ(copy_drop_node->last->related, copy_pick_node->last);
  for (const Activity* activity = copy_pick_node->first.get(); activity != nullptr;
       activity = activity->next.get()) {
    EXPECT_NE(activity->related->related, nullptr);
    EXPECT_EQ(activity->related->related, activity);
  }

  // related_node 指向新链里的 drop node
  EXPECT_EQ(copy_pick_node->first->related_node, copy_drop_node);
  EXPECT_EQ(copy_pick_node->last->related_node, copy_drop_node);

  // 源链未被改写
  EXPECT_EQ(src_pick_node->first->related, src_drop_node->first.get());
  EXPECT_EQ(src_pick_node->first->related_node, src_drop_node);
}
