/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "s_solver.h"

int main() {
  // 允许通过环境变量切换测试用例和日志目录；默认路径仅用于本地调试
  const char* env_case_dir = std::getenv("YUN_CASE_DIR");
  const std::string case_file_path = env_case_dir != nullptr
                                         ? std::string(env_case_dir)
                                         : "/Users/liq/worksapce/code_l/yun-cases/tests/C_SUC_001";
  // 日志目录和等级由调用方传入（封装后的C++包不读取 Config.yaml）
  const char* env_log_dir = std::getenv("YUN_LOG_DIR");
  const std::string log_dir =
      env_log_dir != nullptr ? std::string(env_log_dir) : "/Users/liq/worksapce/code_l/yun/log";
  const std::string log_level = "debug";
  // create solver
  Solver* solver = new Solver(case_file_path, log_dir, log_level);
  // create plan
  solver->solve();

  delete solver;
  return 0;
}
