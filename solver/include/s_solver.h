/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

#include "se_scenario.h"

class Solver {
 public:
  const std::string roo_dir;

  Scenario *scenario;

  Solver(const std::string &roo_dir) : roo_dir(roo_dir) {};

  ~Solver();

  void load_scenario();

  void solve();
};
