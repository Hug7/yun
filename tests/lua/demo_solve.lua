-- DSL 编排脚本的最小形态。
--
-- 这里的 lua 只做"编排"：决定怎么切分订单、每个分区用哪个算法。
-- 所有真正的计算都回调 C++ 注册的方法（全局 solver），lua 自己不算任何东西。
-- 这是刻意的：真实求解的内循环每轮要跑几百万次，lua 不能进去。
--
-- C++ 在加载本脚本前注入了两个全局值：
--   solver     : DemoSolver 实例，携带订单数据和封装好的方法
--   greedy_max : 参数，对应真实项目里的 Parameter，脚本据此做编排决策

-- 求解单个分区：规模小走贪心，规模大走 ALNS
local function solve_partition(name, order_ids)
  local algorithm
  if #order_ids > greedy_max then
    algorithm = "alns"
  else
    algorithm = "greedy"
  end

  -- 两个算法在 C++ 侧签名一致，所以这里只是换个方法名调用
  local cost
  if algorithm == "alns" then
    cost = solver:solve_alns(order_ids)
  else
    cost = solver:solve_greedy(order_ids)
  end

  log(string.format("分区 %s: 订单数=%d, 算法=%s, 成本=%.2f", name, #order_ids, algorithm, cost))

  return { partition = name, algorithm = algorithm, order_count = #order_ids, cost = cost }
end

-- 脚本入口，由 C++ 调用；返回的 table 会被 C++ 读回
function solve()
  log(string.format("订单总数=%d, greedy_max=%d", solver:order_count(), greedy_max))

  -- 按标签把订单切成互相独立的分区，分别规划
  local cold = solver:filter_by_label("COLD")
  local normal = solver:filter_by_label("NORMAL")

  local results = {}
  results[#results + 1] = solve_partition("COLD", cold)
  results[#results + 1] = solve_partition("NORMAL", normal)

  return results
end
