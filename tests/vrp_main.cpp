/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "s_solver.h"

int main() {
  // 允许通过环境变量切换测试用例，默认仍使用本地 case01
  const char* env_case_dir = std::getenv("YUN_CASE_DIR");
  const std::string case_file_path = env_case_dir != nullptr
                                         ? std::string(env_case_dir)
                                         : "/Users/liq/worksapce/code_l/yunpy/test_case/case01";
  // 日志目录和等级由调用方传入（封装后的C++包不读取 Config.yaml）
  const std::string log_dir = "/Users/liq/worksapce/code_l/yun/log";
  const std::string log_level = "debug";
  // create solver
  Solver* solver = new Solver(case_file_path, log_dir, log_level);
  // create plan
  solver->solve();

  delete solver;
  return 0;
}
