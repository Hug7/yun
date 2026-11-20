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

-- 指定近邻数
construct_knn({ neighbor_count = 12 })
```

`construct_knn`的参数必须是`table`；不传参数、传入`nil`或空表时使用默认参数。

#### 参数

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

-- 可选：开启内嵌 local search
alns({
  max_iter = 500,
  local_search = { },  -- 使用 ALNS profile 默认值
})
```

`alns` 的入参必须是`table`（对应 C++ 的`AlnsStrategyConfig`）；不传任何字段（如`alns({})`）时使用全部默认值。

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
| `local_search` | table | 无 | 内嵌邻域搜索配置，**默认关闭**；只有显式写 `enabled = true` 才开启，详见「alns 内嵌 local search」。 |

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
- `local_search`子表解析失败（子表不是`table`、出现未知算子`code`、字段类型错误）时，只打印`warn`并禁用内嵌 local search，不中断`alns`。

#### alns 内嵌 local search

`alns` 支持在内层子迭代中可选地执行一次邻域搜索，用于在 repair 之后、SA 判定之前对候选解做局部改进。
默认关闭：`alns({})` 不会执行邻域搜索，行为与未引入该功能时一致。

其中，`local_search` 键缺失视为关闭。

```lua
alns({
  max_iter = 500,
  local_search = { },
})

alns({
  local_search = {
    epsilon = 1e-6,
    operators = { { code = "LsIntraRouteExchange" } },
  },
})
```

`local_search` 的字段与独立策略步 [local_search](#local_search) 完全一致；未显式配置时使用 `LsParameter{}` 与算子参数结构体的默认值。

内嵌搜索与独立策略步一样跑完整全量搜索（直到一整个 pass 都没有改进），并且会在 ALNS 每个子迭代触发一次，代价可能很高。调试时建议先用较小的问题规模，或只在 `operators` 中保留少量算子。内嵌搜索复用 ALNS 外层构建的订单 K 近邻图做候选剪枝（见 [local_search](#local_search) 的语义与保证）。

运行期前置条件不满足（非 SPMD 装卸模式）或配置的参数非法时，首次执行会打印一次`warn`并禁用内嵌 local search，不中断`alns`。

成功执行时，每次邻域搜索的报告以`debug`级别输出（需要日志级别允许`debug`才能看到）。

### local_search

邻域搜索（local search）在保持订单指派关系不变的前提下，对当前解做局部重排以降低目标值：只搬动、交换或反转已指派订单在车次内的顺序或所属车次，**不插入未指派订单、不改派车型、不增删车次**（v1 禁止把车次搬空）。搜索结束后用改进后的解**覆盖当前`workspace`**，方法无返回值。

SPMD 装卸模式下「可移动单元」= 车次中的卸货（drop）节点：一个停靠点承载一个或多个同站点订单的卸货活动，同一订单的提货活动会随卸货单元一起搬迁。因此 relocate/or-opt 可理解为搬运一段连续订单，2-opt 为反转同一车次内一段连续订单，exchange 为交换两个订单（同车或跨车）。v1 只支持 SPMD，MPSD/MPMD 暂不支持。

#### 调用方式

```lua
-- 默认：6 个邻域实例（同车/跨车的 relocate、or-opt、exchange），跑到局部最优
local_search({})

-- 调整改进阈值、只启用部分邻域（同 code 不同 params 视为不同实例）
local_search({
  epsilon = 1e-6,
  operators = {
    { code = "LsIntraRouteRelocate" },                                                    -- 同车 relocate
    { code = "LsIntraRouteRelocate", params = { min_chain_len = 2, max_chain_len = 3 } }, -- 同车 or-opt
    { code = "LsInterRouteRelocate" },                                                    -- 跨车 relocate
    { code = "LsInterRouteRelocate", params = { min_chain_len = 2, max_chain_len = 3 } }, -- 跨车 or-opt
    { code = "LsIntraRouteExchange" },                                                    -- 同车 exchange
    { code = "LsInterRouteExchange" },                                                    -- 跨车 exchange
    { code = "LsTwoOpt" },                                                                -- 2-opt
  },
})
```

`local_search` 的参数必须是`table`；不传参数、传入`nil`或空表时使用默认参数。该方法应在已有初始解之后调用（例如`construct_knn`之后），
并且要求`workspace`中每个车次非空且可行，否则抛异常并中断脚本。

#### 参数（cfg 顶层字段）

| 字段 | 类型 | 默认值 | 说明 |
| --- | --- | --- | --- |
| `epsilon` | double | `1e-9` | 严格改进阈值：目标值下降超过该值才提交，取值范围>=0。 |
| `improvement` | string | `"best"` | 改进策略：`"best"` 扫完本 pass 全部候选后只提交最优的一个；`"first"` 一旦遇到可行且改进的候选就立即提交并进入下一 pass。只接受这两个值。 |
| `max_evaluated` | int | `0` | 评估次数上限，`0` 表示不限；统计口径是**所有 local search report 的 `evaluated` 之和**。达到上限后不再评估新候选，本 pass 已找到的最优 move 照常提交，然后终止（`reason=max_evaluated`）；预算已用尽后的内嵌调用会直接跳过（`passes=0`）。 |
| `knn_neighbor_count` | int | `10` | KNN 结构剪枝用的近邻数，必须>=0；`0` 表示不做 KNN 剪枝，只保留里程增量剪枝。k 越小剪枝越狠（评估次数更少、解质量取舍更明显）。 |
| `operators` | table | 无 | 邻域算子实例数组，详见下文。缺省或空数组时使用默认 7 个邻域实例。 |

**当前版本没有时间预算、move 数上限、候选数上限**：不配 `max_evaluated` 时搜索一直跑到一整轮（pass）再也找不到改进 move 为止；提前退出只有两种——`improvement = "first"` 找到改进 move 时结束本 pass，或评估次数达到 `max_evaluated`。

#### 算子配置

`operators` 为数组，每个元素是`table`：必含`code`（算子编码字符串），可选`params`（`table`，算子私有参数，缺省时用算子内部默认值）。同一`code`可配置多组不同`params`，视为不同算子实例。`operators`缺省或为空数组时使用默认 7 个邻域实例：同车 relocate、同车 or-opt、同车 exchange、跨车 relocate、跨车 or-opt、跨车 exchange、2-opt。

路线内部（intra-route）与跨路线（inter-route）是各自独立的`code`：前者只在同一车次内搬动，后者只在不同车次之间搬动，因此可以按需只启用其中一侧。

```lua
operators = {
  { code = "LsIntraRouteRelocate" },  -- params 缺省，使用算子默认值
}
```

**LsIntraRouteRelocate / LsInterRouteRelocate（relocate / or-opt）** — 把一段连续可移动单元搬到同一车次的其它位置（intra）或另一个车次（inter）。默认实例中 `min_chain_len = max_chain_len = 1` 为 relocate，`min_chain_len = 2, max_chain_len = 3` 为 or-opt。

| `params` 字段 | 类型 | 默认值 | 说明 |
| --- | --- | --- | --- |
| `min_chain_len` | int | `1` | 最短搬运链长，必须>=1。 |
| `max_chain_len` | int | `1` | 最长搬运链长，必须>=`min_chain_len`；`1`即 relocate，`2~3`即 or-opt。 |

**LsIntraRouteExchange / LsInterRouteExchange（exchange）** — 交换两个可移动单元：同一条车次内的两个单元（intra）或不同车次的两个单元（inter）。没有可配置参数。v1 只交换单单元块，链式交换尚未实现。

**LsTwoOpt（2-opt）** — 反转同一车次内一段连续可移动单元（SPMD 下为 drop 子序列）。

| `params` 字段 | 类型 | 默认值 | 说明 |
| --- | --- | --- | --- |
| `min_span_len` | int | `2` | 最短反转长度，必须>=2。 |
| `max_span_len` | int | `0` | 最长反转长度，`0`表示不限；非 0 时必须>=`min_span_len`。 |

#### 语义与保证

- 只搬动已指派订单：已指派订单集合、车次数、各车次车型、未指派订单在搜索前后不变；会导致空车次的候选直接丢弃。
- 每个候选在受影响车次的副本上执行 move，并调用`Problem::eval_load`全量重评；不可行候选丢弃，可行且目标值下降超过`epsilon`才提交。
- 搜索过程中当前解的目标值不会上升（只接受改进 move）。
- 每个 pass 的起点算子按 pass 序号轮转（第 1 个 pass 从实例 0 开始，第 2 个从实例 1 开始，依次循环），避免总是同一个算子先拿到改进。
- `improvement = "best"`：每个 pass 枚举**所有**算子实例的全部候选，评估完后只提交其中目标值最优的一个（同值取先生成者，保证确定性），然后进入下一个 pass。
- `improvement = "first"`：按轮转顺序逐个算子、逐个候选评估，遇到第一个可行且目标值下降超过`epsilon`的候选就提交，本 pass 立即结束并进入下一 pass；本 pass 剩余候选不再枚举。因此 `first` 通常评估次数更少、单步更贪心，最终解质量可能不如 `best`，`passes` 与 `applied` 也会更多。
- 终止原因有三种：`no_improvement`（本 pass 产出了候选但没有改进 move）、`no_candidate`（所有算子都不再产候选）、`max_evaluated`（评估次数达到 `max_evaluated`）。
- 候选在评估前过两层剪枝：
  1. **里程增量剪枝**：按受影响车次的停车点序列算出 move 生效后的里程变化，里程增加（增量 > 0）的候选直接丢弃；涉事车次当前不可行、或里程无法判定时保留候选交给评估阶段。
  2. **KNN 结构剪枝**：move 执行后**新建的相邻停车点对**里，至少要有一对的订单构成 K 近邻边，否则直接丢弃。relocate 看（目标槽位前驱, 块首）与（块尾, 目标槽位后继），exchange 看两个块互换后各自的左右邻居，2-opt 看（段前, 段尾）与（段首, 段后）；一对相邻点都取不到时（例如只有单个停车点的车次）保留候选。
  
- 里程剪枝阈值是 0，没有可调参数。它假设「里程增加不会带来目标值下降」，因此当硬约束惩罚或计费成本可能因里程增加而下降时，被剪掉的候选里可能含有真实改进；遇到这种情况可以关掉该层剪枝再对比。
- 近邻图由搜索器持有：近邻数取 `knn_neighbor_count`，配成 `0` 时跳过 KNN 剪枝（只跑里程增量剪枝）。`local_search` 在调用时按当前解建图，同一次搜索的所有 pass 复用；`alns` 内嵌搜索在首次使用时建一次图，整个求解过程复用，不会每次局部搜索都重建（不同调用的 `knn_neighbor_count` 以首次建图时为准）。剪枝是启发式：它假设改进只来自相邻停靠点之间的行程缩短，因此被剪掉的候选可能含有非相邻方向的改进，k 越小剪得越狠、解质量换速度的取舍越明显。
- 剪枝后候选规模仍随车次数、单车间停靠点数与链长/跨度组合增长，规模大时单次调用可能很慢。
- 运行结束后通过策略方法日志打印报告摘要，例如（数字为示例，随算例与算子配置变化；`generated = pruned + dist_pruned + evaluated`）：

```text
local_search: obj 3402554.00 -> 3396398.00 (delta -6156.00), passes=3, generated=385, pruned=5, dist_pruned=355, evaluated=25, feasible=14, applied=2, elapsed=0ms, improvement=best, reason=no_improvement
  op[0]=LsIntraRouteRelocate generated=... pruned=... dist_pruned=... evaluated=... feasible=... applied=... delta=...
  op[3]=LsInterRouteExchange generated=... pruned=... dist_pruned=... evaluated=... feasible=... applied=... delta=...
  ...
```

#### 错误处理

- 参数不是`table`、`operators`不是数组、`params`不是`table`、字段类型错误、或`code`不是已注册算子时，抛异常并中断脚本。
- 算子参数取值非法（如`min_chain_len < 1`、`max_chain_len < min_chain_len`、`min_span_len < 2`）、`epsilon`为负/非有限数、或`improvement`不是`"first"`/`"best"`时抛异常。
- `workspace`中存在空车次或不可行车次、或当前装卸模式不是 SPMD 时抛异常并中断脚本（应先跑`construct_knn`等构造/修复方法）。
