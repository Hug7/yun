/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dm_node.h"

// ====== implement of Node ======
Node::Node() : activity_type(ActivityType::NONE), loc(nullptr)
{
    this->ptws = std::vector<std::unique_ptr<TimeWindowPlan>>();
    this->first = nullptr;
    this->last = nullptr;
    this->next = nullptr;
    this->prev = nullptr;
    this->travel_dist = 0;
    this->travel_time = 0;
}

Node::Node(const ActivityType activity_type,
           const Location *loc)
    : activity_type(activity_type),
      loc(loc)
{
    this->ptws = std::vector<std::unique_ptr<TimeWindowPlan>>();
    this->first = nullptr;
    this->last = nullptr;
    this->next = nullptr;
    this->prev = nullptr;
    this->travel_dist = 0;
    this->travel_time = 0;
}

Node::Node(const ActivityType activity_type, 
    const Location *loc, 
    std::unique_ptr<Activity> activity)
    : activity_type(activity_type),
      loc(loc)
{
    this->ptws = std::vector<std::unique_ptr<TimeWindowPlan>>();
    this->first = std::move(activity);
    this->last = this->first.get();
    this->next = nullptr;
    this->prev = nullptr;
    this->travel_dist = 0;
    this->travel_time = 0;
}

void Node::set_travel_dist(long travel_dist)
{
    this->travel_dist = travel_dist;
}

void Node::set_travel_time(long travel_time)
{
    this->travel_time = travel_time;
}

bool Node::hase_next()
{
    return this->next != nullptr;
}

bool Node::hase_prev()
{
    return this->prev != nullptr;
}

void Node::add_front_activity(std::unique_ptr<Activity> activity)
{
    if (this->first != nullptr)
    {
        this->first->prev = activity.get();
        activity->next = std::move(this->first);
        this->first = std::move(activity);
    }
    else
    {
        this->last = activity.get();
        this->first = std::move(activity);
    }
}

void Node::add_back_activity(std::unique_ptr<Activity> activity)
{
    if (this->last != nullptr)
    {
        activity->prev = this->last;
        Activity *activity_ptr = activity.get();
        this->last->next = std::move(activity);
        this->last = activity_ptr;
    }
    else
    {
        this->last = activity.get();
        this->first = std::move(activity);
    }
}

// ====== implement of NodeFactory ======
std::unique_ptr<Node> NodeFactory::create_pick_node(
    const Order *order, std::unique_ptr<Activity> activity)
{
    return std::make_unique<Node>(ActivityType::PICK, order->pick_loc, std::move(activity));
}

std::unique_ptr<Node> NodeFactory::create_drop_node(
    const Order *order, std::unique_ptr<Activity> activity)
{
    return std::make_unique<Node>(ActivityType::DROP, order->drop_loc, std::move(activity));
}

std::pair<std::unique_ptr<Node>, std::unique_ptr<Node>> NodeFactory::create_pair_node(const Order *order)
{
    auto activities_it = ActivityFactory::create_pair_activity(order);
    auto pick_node = std::make_unique<Node>(ActivityType::PICK, order->pick_loc);
    auto drop_node = std::make_unique<Node>(ActivityType::DROP, order->drop_loc);
    pick_node->add_front_activity(std::move(activities_it.first));
    drop_node->add_back_activity(std::move(activities_it.second));

    return std::make_pair(std::move(pick_node), std::move(drop_node));
}

// ====== implement of NodeFactory ======
Node *NodeOps::tail(
    const std::unique_ptr<Node> &head)
{
    if (!head)
    {
        return nullptr;
    }

    Node *cur = head.get();
    while (cur->next)
    {
        cur = cur->next.get();
    }

    return cur;
}

std::unique_ptr<Node> NodeOps::splice_out(
    std::unique_ptr<Node> &from_ptr, Node *to)
{
    auto subchain = std::move(from_ptr); // 摘下整段
    Node *pred = subchain->prev;         // 记住前驱

    from_ptr = std::move(to->next); // 后半段接回前驱

    if (from_ptr)
    {
        from_ptr->prev = pred; // 后半段头节点的 prev
    }

    subchain->prev = nullptr; // 独立链无前驱
    return subchain;
}

void NodeOps::splice_in_after(
    Node *after, std::unique_ptr<Node> chain)
{
    if (!chain)
    {
        return;
    }

    Node *t = tail(chain);

    t->next = std::move(after->next); // chain 尾部接原后继
    if (t->next)
    {
        t->next->prev = t;
    }

    chain->prev = after;            // chain 头部指回 after
    after->next = std::move(chain); // after 接 chain
}

std::unique_ptr<Node> NodeOps::reverse_chain(
    std::unique_ptr<Node> chain)
{
    if (!chain || !chain->next)
    {
        return chain;
    }
    std::unique_ptr<Node> prev_p;
    std::unique_ptr<Node> curr = std::move(chain);
    while (curr)
    {
        auto nxt = std::move(curr->next);
        curr->next = std::move(prev_p);
        if (curr->next)
        {
            curr->next->prev = curr.get(); // 翻转后 prev 也跟着翻
        }
        prev_p = std::move(curr);
        curr = std::move(nxt);
    }

    if (prev_p)
    {
        prev_p->prev = nullptr;
    }
    return prev_p;
}
