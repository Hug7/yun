/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstdlib>
#include <iostream>
#include <string>

#include "s_solver.h"

int main(int argc, char** argv) {
  std::string case_dir;
  std::string log_dir;
  std::string log_level = "debug";

  if (argc >= 2) {
    case_dir = argv[1];
  }
  if (argc >= 3) {
    log_dir = argv[2];
  }
  if (argc >= 4) {
    log_level = argv[3];
  }

  if (case_dir.empty()) {
    if (const char* env = std::getenv("YUN_CASE_DIR")) {
      case_dir = env;
    }
  }
  if (log_dir.empty()) {
    if (const char* env = std::getenv("YUN_LOG_DIR")) {
      log_dir = env;
    }
  }
  if (log_dir.empty()) {
    log_dir = "log";
  }

  if (case_dir.empty()) {
    std::cerr << "Usage: vrp_exec <case_dir> [log_dir] [log_level]\n";
    return 2;
  }

  Solver solver(case_dir, log_dir, log_level);
  solver.solve();
  return 0;
}
