/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "s_solver.h"

#include <memory>

#include "pr_vrp.h"
#include "standard_csv_reader.h"
#include "pr_problem.h"


// ====== implement of Load Solver ======
Solver::~Solver() {
  // release scenario
  delete this->scenario;
}

void Solver::load_scenario() {
  auto reader = std::make_unique<StandardCsvReader>(this->roo_dir);

  this->scenario = reader->loading_scenario();
}

void Solver::create_problem() {
  // TODO 先默认创建 VRP
  this->problem = new ProblemVRP(this->scenario);
}

void Solver::precheck() {
  
}
