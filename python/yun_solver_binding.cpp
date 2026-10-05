/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Solver 的 pybind11 绑定, 供 yunpy 项目进程内调用
 *
 * 对应 case 目录结构见 yunpy/test_case, Python 侧入口为 yunpy.solver.solve()。
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <utility>

#include "pdm_context.h"
#include "s_solver.h"
#include "visual_schema.h"

namespace py = pybind11;

namespace {

/**
 * @brief 求解产物的 Python 侧 key -> 输出文件名
 */
constexpr std::pair<const char*, const char*> RESULT_ENTRIES[] = {
    {"loads", LOADS_FILE_NAME},
    {"locations", LOAD_LOCATIONS_FILE_NAME},
    {"unassigned", UNASSIGNED_CARGO_ORDERS_FILE_NAME},
    {"infeasible", INFEASIBLE_CARGO_ORDERS_FILE_NAME},
};

/**
 * @brief 收集本次求解的产物路径, 未产出的 key 记为 std::nullopt(Python 侧为 None)
 *
 * 纯 std::filesystem 扫描, 不触碰 Python 对象, 因此可以在释放 GIL 的状态下调用。
 */
std::map<std::string, std::optional<std::string>> collect_results(const std::string& output_dir) {
  std::map<std::string, std::optional<std::string>> results;
  for (const auto& [key, file_name] : RESULT_ENTRIES) {
    const std::filesystem::path path = std::filesystem::path(output_dir) / file_name;
    if (std::filesystem::exists(path)) {
      results[key] = path.string();
    } else {
      results[key] = std::nullopt;
    }
  }
  return results;
}

}  // namespace

PYBIND11_MODULE(_core, m) {
  m.doc() = "Solver binding for the yun VRP/ALNS solver.\n"
            "\n"
            "Known behaviour: the ALNS stage does NOT write its best solution back to the\n"
            "workspace, so the exported loads are the KNN construction result. The max_iter\n"
            "setting in strategy.lua therefore does not affect the output.";

  py::class_<Solver>(m, "Solver")
      .def(py::init<std::string, const std::string&, const std::string&>(), py::arg("root_dir"),
           py::arg("log_dir"), py::arg("log_level"),
           R"doc(构造 solver, 立即从 root_dir 读取全部场景 CSV 并创建输出目录。

Args:
    root_dir: 案例目录, 需包含 22 个场景 CSV 与 strategy.lua, 且必须可写
    log_dir: 额外日志目录; 传空串则跳过该输出
    log_level: 日志等级

Raises:
    RuntimeError: 场景读取失败或 strategy.lua 缺失
)doc",
           py::call_guard<py::gil_scoped_release>())
      .def_property_readonly("output_dir", [](const Solver& solver) { return solver.context->output_dir; },
                             R"doc(本次请求的输出目录: <root_dir>/output/<request_id>)doc")
      .def(
          "solve",
          [](Solver& solver) {
            solver.solve();
            return collect_results(solver.context->output_dir);
          },
          R"doc(执行一次求解。

Returns:
    dict[str, str | None]: 产物路径, key 为 loads/locations/unassigned/infeasible,
    未产出的 key 为 None。precheck 拦截时 loads 为 None。

Note:
    同一个 Solver 可以重复调用, 每次会重新构建订单池与工作台。
)doc",
          py::call_guard<py::gil_scoped_release>());
}
