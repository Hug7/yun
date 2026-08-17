/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <vector>

#include "dm_order.h"

class VehicleResource {
 public:
  const std::vector<int> usable;

  const int len;

  VehicleResource(std::vector<int>& usable) : usable(usable), len(usable.size()) {};
};

class VehicleResourceUsage {
 public:
  using UPtr = std::unique_ptr<VehicleResourceUsage>;

  VehicleResource* resource;

  std::vector<int> usage;

  VehicleResourceUsage(VehicleResource* resource) : resource(resource), usage(resource->len, 0) {};
};

class OrderResource {
 public:
  const std::vector<Order*> orders;

  const int len;

  OrderResource(std::vector<Order*>& orders) : orders(orders), len(orders.size()) {};
};
