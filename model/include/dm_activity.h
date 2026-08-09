/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include "dm_predefine.h"
#include "dm_order.h"

/**
 * @brief activity is action for order in node
 */
class Activity
{
public:
    /**
     * @brief activity type
     */
    const ActivityType activity_type;
    /**
     * @brief order of activity
     */
    const Order *order;
    /**
     * @brief previous activity
     */
    Activity *prev;
    /**
     * @brief next activity
     */
    std::unique_ptr<Activity> next;
    /**
     * @brief related activity
     */
    Activity *related;
    /**
     * @brief related node
     */
    Node *related_node;

    Activity();

    Activity(const ActivityType activity_type, const Order *order);

    void set_prev(Activity *prev);

    void set_next(std::unique_ptr<Activity> next);

    void set_related(Activity *related);

    void set_related_node(Node *related_node);

    bool hase_next();

    bool hase_prev();
};

namespace ActivityFactory
{
    std::pair<std::unique_ptr<Activity>, std::unique_ptr<Activity>> create_pair_activity(const Order *order);
}
