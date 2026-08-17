/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dm_activity.h"

// ====== implement of Activity ======
Activity::Activity() : activity_type(ActivityType::NONE), order(nullptr) {
  this->prev = nullptr;
  this->next = nullptr;
  this->related = nullptr;
  this->related_node = nullptr;
}

Activity::Activity(const ActivityType activity_type, const Order* order)
    : activity_type(activity_type), order(order) {
  this->prev = nullptr;
  this->next = nullptr;
  this->related = nullptr;
  this->related_node = nullptr;
}

void Activity::set_prev(Activity* prev) { this->prev = prev; }

void Activity::set_next(Activity::UPtr next) { this->next = std::move(next); }

void Activity::set_related(Activity* related) { this->related = related; }

void Activity::set_related_node(Node* related_node) { this->related_node = related_node; }

bool Activity::hase_next() { return this->next != nullptr; }

bool Activity::hase_prev() { return this->prev != nullptr; }

// ====== implement of ActivityFactory ======
std::pair<Activity::UPtr, Activity::UPtr> ActivityFactory::create_pair_activity(
    const Order* order) {
  auto pick_activity = std::make_unique<Activity>(ActivityType::PICK, order);
  auto drop_activity = std::make_unique<Activity>(ActivityType::DROP, order);
  pick_activity->related = drop_activity.get();
  drop_activity->related = pick_activity.get();

  return std::make_pair(std::move(pick_activity), std::move(drop_activity));
}
