/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "s_solver.h"

int main() {
  const std::string case_file_path = "/Users/liq/worksapce/code_l/yunpy/test_case/case1";
  // create solver
  Solver* solver = new Solver(case_file_path);
  // create plan
  solver->solve();

  delete solver;
  return 0;
}
