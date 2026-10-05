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
| `neighbor_count` | integer | `8` | 每个订单构造近邻关系时保留的最佳复合`rank`数量。应传入正整数；当前实现未对其取值范围做额外校验。 |

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
  tasks = 4,
})

-- 完整配置：算法参数 + ruin/repair 算子
alns({
  max_iter = 500,
  max_unimproved_iter = 200,
  tasks = 4,
  ruin_operator_params = {
    { code = "RuinRandomLoad", params = { select_load_min_rate = 0.1, select_load_max_rate = 0.3 } },
    { code = "RuinRandomLoad", params = { select_load_min_rate = 0.4, select_load_max_rate = 0.6 } },
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
| `tasks` | integer | `4` | 并行线程数。必须 `> 0`；若 `>=` 机器逻辑核数则自动降级为 `核数 - 1` 并打 warning。当前版本每次`alns`调用都会新建并销毁线程池。 |
| `max_iter` | integer | `200` | 最大迭代次数（外层 ALNS 主循环）。 |
| `max_sub_iter` | integer | `10` | 最大子迭代次数。 |
| `max_unimproved_iter` | integer | `100` | 连续无改进次数达到该值时提前终止搜索。 |
| `max_running_time_sec` | integer | `600` | 最大运行时长（秒），到时终止搜索。 |
| `operator_learning_factor` | number | `0.35` | 算子权重自适应学习因子。必须满足 `0 < x < 1`，否则抛异常。 |
| `operator_initial_weight` | number | `100.0` | 算子组合初始权重。必须 `>= 1`，否则抛异常。 |
| `initial_temperature` | number | `10000.0` | 模拟退火初始温度。 |
| `min_temperature` | number | `0.97` | 模拟退火最小温度。 |
| `annealing_factor` | number | `0.97` | 退火系数（每轮温度乘以该系数）。 |
| `default_random_seed` | integer | `37` | 默认随机种子。 |
| `ruin_operator_params` | table | 无 | ruin 算子配置数组，详见下文。 |
| `repair_operator_params` | table | 无 | repair 算子配置数组，详见下文。 |

终止条件：满足`max_iter`、`max_unimproved_iter`、`max_running_time_sec`三者之一即停止。

#### ruin / repair 算子

`ruin`（破坏）与`repair`（修复）算子各以数组形式配置，算法会将所有 ruin 与 repair 算子做**笛卡尔积**展开为算子组合，每个组合按`operator_initial_weight`初始化权重，并在搜索过程中依据`operator_learning_factor`自适应调整。

数组中每个元素为`table`，必含字段`code`（算子编码字符串），可选字段`params`（`table`，算子私有参数，缺省时用算子内部默认值）。同一`code`可配置多组不同`params`，视为不同算子实例。

```lua
ruin_operator_params = {
  { code = "RuinRandomLoad", params = { select_load_min_rate = 0.1, select_load_max_rate = 0.3 } },
  { code = "RuinRandomLoad" },  -- params 缺省，使用算子默认值
}
```

#### 支持的算子

**RuinRandomLoad（ruin）** — 随机移除整条车次。

| `params` 字段 | 类型 | 默认值 | 说明 |
| --- | --- | --- | --- |
| `select_load_min_rate` | number | `0.1` | 被移除车次数占当前车次数的最小比例，区间 `[0, 1]`。 |
| `select_load_max_rate` | number | `0.4` | 被移除车次数的最大比例，区间 `[0, 1]`；必须 `>= select_load_min_rate`。 |
| `select_load_random_seed` | integer | `37` | 选择车次的随机种子。 |

> 注：当前`RuinRandomLoad`只挑整条车次、不填充 activities，因此修复算子总是从被移除订单重新构造（走"整 load 删除"分支）。

**RepairGreedy（repair）** — 贪心修复。

| `params` 字段 | 类型 | 默认值 | 说明 |
| --- | --- | --- | --- |
| `random_seed` | integer | `37` | 随机选择候选方案的随机种子。 |
| `top_rate` | number | `0.2` | 从修复矩阵中按代价排序取前 `top_rate` 比例的候选方案，再随机选一个落实；区间 `[0, 1]`。 |

#### 错误处理

- `ruin_operator_params` / `repair_operator_params` 中元素缺少`code`字段、或`code`不是已注册算子时，抛异常并中断脚本加载。
- 算子参数取值非法（如比例越界、`min_rate > max_rate`、学习因子不在`(0,1)`）时抛异常。
- `params`存在但不是`table`时抛异常。
