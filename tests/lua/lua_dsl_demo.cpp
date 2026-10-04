/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

// sol2 + lua 双向通信 demo，是将来 DSL 的最小可运行骨架。
//
// 运作流程与真实 DSL 一致：
//   C++ 组织输入数据 -> 注册 C++ 方法 -> 加载并调用 lua 脚本
//   -> lua 回调 C++ 完成编排 -> C++ 取回结果
//
// lua 只负责编排（按标签切分订单、为每个分区挑算法），所有计算都留在 C++。
// 这是刻意的：真实求解的内循环每轮要跑几百万次，lua 绝不能进去。
//
// 编译：cmake --build build/Debug --target lua_dsl_demo
// 运行：./build/Debug/tests/lua_dsl_demo

#include <iostream>
#include <sol/sol.hpp>
#include <string>
#include <utility>
#include <vector>

// 脚本目录由 CMake 注入，避免依赖运行时的工作目录；手工编译时回退到当前目录
#ifndef YUN_LUA_DEMO_DIR
#define YUN_LUA_DEMO_DIR "."
#endif

namespace {

/**
 * @brief demo 用的订单，对应真实项目里的 Order，只保留编排需要的字段
 */
struct DemoOrder {
  int id;
  std::string label;  // COLD / NORMAL
  double weight;
};

/**
 * @brief C++ 侧封装好的能力，将来对应真实的过滤方法与算法
 * @details lua 通过 sol2 注册的方法回调到这里，脚本本身不做任何计算
 */
class DemoSolver {
 public:
  explicit DemoSolver(std::vector<DemoOrder> orders) : orders(std::move(orders)) {}

  /**
   * @brief 订单总数
   */
  [[nodiscard]] int order_count() const { return static_cast<int>(this->orders.size()); }

  /**
   * @brief 过滤：返回带指定标签的订单 id
   * @details 返回的 vector 由 sol2 自动转成 lua table，脚本侧可以直接用 # 取长度，
   *          也可以原样传回 solve_greedy / solve_alns
   */
  [[nodiscard]] std::vector<int> filter_by_label(const std::string& label) const {
    std::vector<int> ids;
    for (const auto& order : this->orders) {
      if (order.label == label) {
        ids.push_back(order.id);
      }
    }
    return ids;
  }

  /**
   * @brief 算法一：贪心，单位成本更高
   */
  [[nodiscard]] double solve_greedy(const std::vector<int>& order_ids) const {
    return this->total_weight(order_ids) * 12.0 + static_cast<double>(order_ids.size()) * 100.0;
  }

  /**
   * @brief 算法二：ALNS，单位成本更低
   * @details 与 solve_greedy 签名完全一致，这正是将来统一 Algorithm 契约的雏形：
   *          算法可替换的前提是接口一致，lua 才能只换名字不换调用方式
   */
  [[nodiscard]] double solve_alns(const std::vector<int>& order_ids) const {
    return this->total_weight(order_ids) * 9.5 + static_cast<double>(order_ids.size()) * 80.0;
  }

 private:
  /**
   * @brief 汇总指定订单的重量
   * @details demo 规模下线性查找即可，真实实现应走索引
   */
  [[nodiscard]] double total_weight(const std::vector<int>& order_ids) const {
    double total = 0.0;
    for (const int id : order_ids) {
      for (const auto& order : this->orders) {
        if (order.id == id) {
          total += order.weight;
          break;
        }
      }
    }
    return total;
  }

  std::vector<DemoOrder> orders;
};

}  // namespace

int main() {
  // ====== step 1: C++ 组织输入数据 ======
  // 3 个冷链 + 6 个普通，配合下面注入的 greedy_max=4，可以让两个分区分别走到不同算法
  const std::vector<DemoOrder> orders{
      {0, "COLD", 12.5},   {1, "COLD", 8.0},    {2, "COLD", 20.0},
      {3, "NORMAL", 5.5},  {4, "NORMAL", 30.0}, {5, "NORMAL", 7.5},
      {6, "NORMAL", 18.0}, {7, "NORMAL", 9.0},  {8, "NORMAL", 22.5},
  };
  DemoSolver solver(orders);

  // ====== step 2: 起 lua 虚拟机，把 C++ 能力注册进去 ======
  sol::state lua;
  lua.open_libraries(sol::lib::base, sol::lib::string, sol::lib::table);

  // 注册自由函数：脚本里的 log() 实际走 C++ 的输出，这是最简单的注册形式
  lua.set_function("log", [](const std::string& msg) { std::cout << "[lua] " << msg << "\n"; });

  // 注册 usertype：把 DemoSolver 的方法暴露给脚本
  // sol::no_constructor 表示该类型只能在 C++ 侧构造，脚本不能自己 new，
  // 这样对象所有权始终留在 C++，避免 lua GC 和 C++ 析构打架
  lua.new_usertype<DemoSolver>("DemoSolver", sol::no_constructor, "order_count",
                               &DemoSolver::order_count, "filter_by_label",
                               &DemoSolver::filter_by_label, "solve_greedy",
                               &DemoSolver::solve_greedy, "solve_alns", &DemoSolver::solve_alns);

  // 传入已经构造好的实例；裸指针在 sol2 里是非拥有引用，生命周期由 C++ 负责
  lua.set("solver", &solver);
  // 注入参数：对应真实项目里的 Parameter，脚本据此做编排决策
  lua.set("greedy_max", 4);

  // ====== step 3: 加载 DSL 脚本 ======
  const std::string script_path = std::string(YUN_LUA_DEMO_DIR) + "/lua/demo_solve.lua";
  std::cout << "[cpp] lua version: " << LUA_VERSION << "\n";
  std::cout << "[cpp] 加载脚本: " << script_path << "\n";

  // safe_script_file + pass_on_error：脚本出错时不抛异常，统一在下面判断，
  // 真实的 DSL 也必须这样处理，否则一个脚本语法错误会直接掀掉整个求解进程
  const sol::protected_function_result load_result =
      lua.safe_script_file(script_path, sol::script_pass_on_error);
  if (!load_result.valid()) {
    const sol::error err = load_result;
    std::cerr << "[cpp] 脚本加载失败: " << err.what() << "\n";
    return 1;
  }

  // ====== step 4: 调用脚本入口，把结果取回 C++ ======
  const sol::protected_function solve = lua["solve"];
  const sol::protected_function_result call_result = solve();
  if (!call_result.valid()) {
    const sol::error err = call_result;
    std::cerr << "[cpp] solve() 执行失败: " << err.what() << "\n";
    return 1;
  }

  const sol::table results = call_result;
  double total_cost = 0.0;
  std::cout << "[cpp] ---- 求解结果 ----\n";
  // 按下标遍历而不是 pairs：lua 的 next 顺序不保证，结果输出必须稳定
  for (std::size_t i = 1; i <= results.size(); ++i) {
    const sol::table row = results[i];
    const std::string partition = row["partition"];
    const std::string algorithm = row["algorithm"];
    const int order_count = row["order_count"];
    const double cost = row["cost"];
    total_cost += cost;
    std::cout << "[cpp] 分区=" << partition << " 订单数=" << order_count << " 算法=" << algorithm
              << " 成本=" << cost << "\n";
  }
  std::cout << "[cpp] 总成本=" << total_cost << "\n";

  return 0;
}
