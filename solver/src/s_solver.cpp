/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "s_solver.h"

#include <memory>

#include "standard_csv_reader.h"

// ====== implement of Load Solver ======
Solver::~Solver() {
  // release scenario
  delete this->scenario;
}

void Solver::load_scenario() {
  auto reader = std::make_unique<StandardCsvReader>(this->roo_dir);

  this->scenario = reader->loading_scenario();
}
