# DSL用户手册

`DSL`(domain specific language) 是项目面向求解策略提供的 `Lua` 编排接口。

## 方法-基本

### log

将日志功能封装供策略灵活打印执行`DSL`脚本过程中关键信息。包含四个日志等级：

#### log
`log(string msg)`: 打印`info`级别的msg日志.

`log(string msg, int log_level)`: 打印指定级别的msg日志：
* `log_level=1`: 打印`debug`级别的msg日志;
* `log_level=2`: 打印`warning`级别的msg日志;
* `log_level=3`: 打印`error`级别的msg日志;
* 其它: 打印`info`级别的msg日志;

## 方法-约束

## 方法-车次

## 方法-优化

### construct_knn

K-Nearest Neighbor(`knn`)启发式构造算法。
该方法基于当前`workspace`中的已指派订单和未指派订单构造有向`knn-graph`，再按订单之间的近邻`rank`尝试将未指派订单插入已有车次；
如果没有可插入位置，则选择最小的最晚卸货时间的订单构造新的车次。执行结果直接覆盖当前`workspace`，方法无返回值。

#### 调用方式

```lua
-- 使用默认近邻数 8
construct_knn()

-- 等价于使用默认值
construct_knn(KnnParameter())

-- 指定近邻数
construct_knn(KnnParameter(12))
```

`construct_knn`的参数必须是`KnnParameter`对象；不传参数或传入`nil`时使用默认参数。

#### KnnParameter

`KnnParameter`用于设置`construct_knn`的近邻数量：

```lua
local parameter = KnnParameter()
parameter.neighbor_count = 12
construct_knn(parameter)
```

也可以直接调用`KnnParameter(neighbor_count)`进行初始化。

| 字段 | 类型 | 默认值 | 说明 |
| --- | --- | --- | --- |
| `neighbor_count` | int | `8` | 每个订单构造近邻关系时保留的最佳复合`rank`数量。应传入正整数；当前实现未对其取值范围做额外校验。 |

#### knn-graph

`knn-graph`包含当前车次中的订单和未指派订单，并为每个订单记录其可单向连接的近邻订单及`rank`值。两个订单之间存在候选关系时，按以下方式计算：

1. 取两个订单可用车型的交集；如果没有公共可用车型，则不建立候选关系。
2. 分别在公共可用车型的订单提货点、卸货点距离矩阵上计算平均距离。
3. 将提货距离和卸货距离分别升序排名，两个排名相加得到订单对的复合`rank`，数值越小表示越近。
4. 保留复合`rank`最小的`neighbor_count`组不同值。具有相同复合`rank`的订单会同时保留，因此每个订单的近邻数可能超过`neighbor_count`。

当`LoadUnloadPolicyType`为`FILO`时，提货距离按“候选订单提货点 -> 当前订单提货点”的方向计算；其它策略按“当前订单提货点 -> 候选订单提货点”的方向计算。

#### 构造流程

1. 从`workspace`生成解，并将已有车次作为候选车次。
2. 在所有“未指派订单可以追加到候选车次末尾”的可行组合中，优先选择复合`rank`最小的组合；插入后重新评估车次约束并选择最佳车辆。
3. 如果未指派订单无法插入任何候选车次，则从有卸货时间窗的订单中选择最晚卸货时间最小者作为种子订单，构造新的候选车次；种子订单无法成车时保留为未指派订单。
4. 对已确认无法接收任何未指派订单的候选车次提前归档，减少重复约束计算。
5. 所有候选车次处理完成后，将结果覆盖回`workspace`。无法构造或插入的订单保留在未指派订单中。

### alns

ALNS（Adaptive Large Neighborhood Search，自适应大邻域搜索）是项目提供的元启发式优化接口。`alns(cfg)` 读取当前`workspace`中的车次与未指派订单作为初始解，在 DSL 配置的算法参数与 ruin/repair 算子组合下搜索更优解，搜索结束后用最优解**覆盖当前`workspace`**，方法无返回值。

`alns` 应在脚本的`solve()`内调用，通常放在`construct_knn`等构造启发式之后作为提升阶段。

#### 调用方式

```lua
-- 使用全部默认参数运行一次 ALNS
alns({})

-- 自定义迭代次数与线程数
alns({
  max_iter = 500,
  worker_count = 4,
})

-- 完整配置：算法参数 + ruin/repair 算子
alns({
  max_iter = 500,
  max_unimproved_iter = 200,
  worker_count = 4,
  ruin_operator_params = {
    { code = "RuinRandomRemovalLoad", params = { select_load_min_rate = 0.1, select_load_max_rate = 0.3 } },
    { code = "RuinRandomRemovalLoad", params = { select_load_min_rate = 0.4, select_load_max_rate = 0.6 } },
  },
  repair_operator_params = {
    { code = "RepairGreedy", params = { top_rate = 0.2 } },
  },
})
```

`alns` 的入参必须是`table`（对应 C++ 的`AlnsParameter`）；不传任何字段（如`alns({})`）时使用全部默认值。

#### 算法参数（cfg 顶层字段）

| 字段 | 类型 | 默认值 | 说明 |
| --- | --- | --- | --- |
| `worker_count` | int | `-1` | 开启执行器的并行数。`-1` 表示自动使用默认硬件并发，其余取值必须为正整数，`0` 与其他负数会抛异常。 |
| `max_iter` | int | `200` | 最大迭代次数（外层 ALNS 主循环），取值范围[1,10000]。 |
| `max_sub_iter` | int | `10` | 最大子迭代次数，取值范围[1,40]。 |
| `max_unimproved_iter` | int | `100` | 连续无改进次数达到该值时提前终止搜索，取值范围>=1。 |
| `max_stagnate_iter` | int | `20` | 最优解超过`最大停滞迭代次数`没有更新，则触发重置当前解，取值范围>=1。 |
| `print_log_interval_iter` | int | `10` | 打印日志的间隔迭代次数，取值范围>=1。 |
| `max_running_time_sec` | int | `600` | 最大运行时长（秒），到时终止搜索，取值范围>=1。 |
| `operator_learning_factor` | double | `0.35` | 算子权重的反应因子 ρ（Ropke-Pisinger）。每个 segment 结束时按 `w ← (1-ρ)·w + ρ·π/θ` 更新一次。必须满足 `0 < x < 1`，否则抛异常。 |
| `operator_initial_weight` | double | `1.0` | 算子初始权重。ruin 与 repair 两组权重都从该值起步，与 σ 分数同量级。必须 `> 0`，否则抛异常。 |
| `operator_min_weight` | double | `0.05` | 算子权重下限。权重更新后取 `max(weight, operator_min_weight)`，避免算子被永久淘汰。必须 `> 0` 且小于 `operator_initial_weight`，否则抛异常。 |
| `operator_weight_update_interval_iter` | int | `5` | 算子权重的更新间隔（外层迭代数）。每个 segment 内累计各算子的得分与使用次数，段末统一更新；必须 `0 < x <= 20`，否则抛异常。 |
| `initial_temperature` | double | `2.0e-2` | 初始**相对**温度 τ₀（无量纲）。接受概率为 `exp(-Δf / (τ·f0))`，`f0` 是初始解的总目标值，故该值不随算例目标值量级变化；建议 `1e-3 ~ 2e-1`，大于 `1` 会告警。`initial_temperature`必须大于 `min_temperature`否则抛异常。 |
| `min_temperature` | double | `2.0e-5` | 最小相对温度 τ_end（无量纲），温度取到该值后不再下降。 |
| `annealing_factor` | double | `0.0` | 冷却系数 α，需满足 `0 < α < 1`；置为 `<= 0` 时由 `α = (τ_end / τ₀) ^ (1 / max_iter)` 自动推导，保证最后一轮恰好降到 τ_end。 |
| `default_random_seed` | int | `37` | 默认随机种子。 |
| `print_operator_statistics` | bool | `false` | 运行结束后是否打印 ruin×repair 二维算子统计。 |
| `ruin_operator_params` | table | 无 | ruin 算子配置数组，详见下文。 |
| `repair_operator_params` | table | 无 | repair 算子配置数组，详见下文。 |

终止条件：满足`max_iter`、`max_unimproved_iter`、`max_running_time_sec`三者之一即停止。

#### ruin / repair 算子

`ruin`（破坏）与`repair`（修复）算子各以数组形式配置。算法分别为两组算子维护一维权重（都由`operator_initial_weight`初始化），
每轮迭代先按 ruin 权重、再按 repair 权重各转一次轮盘，抽出一个组合执行；某个组合被抽中的概率是该 ruin 与 repair 权重之积。
每个 segment（`operator_weight_update_interval_iter` 轮）内按算子累计得分与使用次数求均值，段末按反应因子
`operator_learning_factor` 更新到对应的 ruin 与 repair 权重上，并限制在`operator_min_weight`以上，避免算子被永久淘汰。

算子得分采用 Ropke-Pisinger 的 σ 分数：

| 判定结果 | 分数 |
| --- | --- |
| 得到新的全局最优解 | `33` |
| 优于当前解 | `9` |
| 被概率接受但劣于当前解 | `13` |
| 被拒绝 | `0` |

其中「被概率接受」的奖励高于「优于当前解」是刻意设计，用于鼓励多样化；绝对尺度与权重初值 `1.0`、反应因子 `0.1` 配套。

开启`print_operator_statistics`时，整个 alns 运行结束后会打印一张 `[ruin][repair]` 二维统计表（每个组合的分数累计、使用次数、平均分）以及 ruin和repair 的单独汇总。
统计只用于观测，不参与权重计算。

ruin 或 repair 算子数组为空时，`alns` 会使用默认算子（
ruin 为 `RuinRandomRemovalLoad`、`RuinRandomRemovalOrder`、`RuinWorstRemovalOrder`，
repair 为 `RepairGreedy`、`RepairRegretK`，参数取各自默认值）。
每次执行算法前，`alns` 都会打印实际使用的算子及其参数，便于确认默认兜底是否生效。

数组中每个元素为`table`，必含字段`code`（算子编码字符串），可选字段`params`（`table`，算子私有参数，缺省时用算子内部默认值）。
同一`code`可配置多组不同`params`，视为不同算子实例。

```lua
ruin_operator_params = {
  { code = "RuinRandomRemovalLoad", params = { select_load_min_rate = 0.1, select_load_max_rate = 0.3 } },
  { code = "RuinRandomRemovalLoad" },  -- params 缺省，使用算子默认值
}
```

#### 支持的算子

**RuinRandomRemovalLoad（ruin）** — 随机移除整车。

| `params` 字段 | 类型 | 默认值 | 说明 |
| --- | --- | --- | --- |
| `select_load_min_rate` | double | `0.1` | 被移除车次数占当前车次数的最小比例，区间 `[0, 1]`。 |
| `select_load_max_rate` | double | `0.4` | 被移除车次数的最大比例，区间 `[0, 1]`；必须 `>= select_load_min_rate`。 |
| `select_load_random_seed` | int | `37` | 选择车次的随机种子。 |

**RuinRandomRemovalOrder（ruin）** — 随机移除订单。

| `params` 字段 | 类型 | 默认值 | 说明 |
| --- | --- | --- | --- |
| `select_order_min_rate` | double | `0.1` | 被移除订单数占当前已分配订单数的最小比例，区间 `[0, 1]`。 |
| `select_order_max_rate` | double | `0.4` | 被移除订单数的最大比例，区间 `[0, 1]`；必须 `>= select_order_min_rate`。 |
| `select_order_random_seed` | int | `37` | 选择订单的随机种子。 |

**RuinWorstRemovalOrder（ruin）** — 移除最差订单。

按「把该订单从所属车次摘掉后，该车次目标值的下降量」排序，优先移除收益最大的订单。目标值口径与 repair 一致，
包含硬约束罚分、软约束罚分与计费成本，因此看重的不只是距离，也包括时间窗、可用车辆与计费项的影响。

排序后按 `pos = ⌊n · y^p⌋`（`y ~ U(0,1)`）随机取位：`p` 越大越集中在收益最高者，`p = 1` 等价于在候选池上均匀随机。

| `params` 字段 | 类型 | 默认值 | 说明 |
| --- | --- | --- | --- |
| `select_order_min_rate` | double | `0.1` | 被移除订单数占当前已分配订单数的最小比例，区间 `[0, 1]`。 |
| `select_order_max_rate` | double | `0.3` | 被移除订单数的最大比例，区间 `[0, 1]`；必须 `>= select_order_min_rate`。 |
| `select_order_random_seed` | int | `37` | 选择订单的随机种子。 |
| `select_order_noise_exponent` | double | `3.0` | 贪心指数 `p`，必须 `>= 1`；越大越偏向收益最高的订单。 |

**RepairGreedy（repair）** — 贪心修复。

| `params` 字段 | 类型 | 默认值 | 说明 |
| --- | --- | --- | --- |
| `random_seed` | int | `37` | 随机选择候选方案的随机种子。 |
| `top_rate` | double | `0.2` | 从修复矩阵中按代价排序取前 `top_rate` 比例的候选方案，再随机选一个落实；区间 `[0, 1]`。 |

**RepairRegretK（repair）** — Regret-K 后悔式修复。

| `params` 字段 | 类型 | 默认值 | 说明 |
| --- | --- | --- | --- |
| `regret_k` | int | `2` | 后悔值阶数 K，必须 `>= 2`。 |

每轮对每个未指派 segment 取最多 K 个最优插入方案，按以下三级优先级选择：

1. 只有 1 个可行方案：最高优先级；同层内按最优 `delta` 最小者优先。
2. 有 2 ~ K-1 个可行方案：先按实际候选计算基础 regret `Σ(c_i - c_1)`，再加稀缺奖励 `(K-n) × (c_n - c_1 + 1)`。
3. 有至少 K 个可行方案：计算标准 Regret-K `Σ_{i=2..K}(c_i - c_1)`。

其中 `n` 为该 segment 的可行方案数，`c_1 ≤ ... ≤ c_n` 为按 `delta` 升序排列的候选方案。同层内选择 regret 最大的 segment，并落实其最优插入方案；当所有未指派 segment 都没有可行方案时，新建车次兜底。

#### 错误处理

- `ruin_operator_params` / `repair_operator_params` 中元素缺少`code`字段、或`code`不是已注册算子时，抛异常并中断脚本加载。
- 算子参数取值非法（如比例越界、`min_rate > max_rate`、学习因子不在`(0,1)`）时抛异常。
- `params`存在但不是`table`时抛异常。
