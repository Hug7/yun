# 参数文档

> 考虑友好性和对大模型的适配使用`.yaml`文件作为参数的载体。  
> `Parameter.yaml`为固定的参数文件名。

## 通用约定

- 每个参数都有默认值，即非必填。

## 参数明细

### 规划时间范围
`start_time`和`end_time`分别代表着本次规划的时间范围，用于控制决策时空属性。
日期格式遵循 `"%Y-%m-%d %H:%M"`。

`start_time`的默认值为`1970-01-01 00:00`。

`end_time`的默认值为`2096-11-16 02:00`。

demo如下所示:

```yaml
start_time: "2026-10-14 00:00"
end_time: "2026-10-14 23:59"
```

### strategy_file
`strategy_file`策略文件名，默认值 `Strategy`。demo如下所示:

```yaml
strategy_file: Strategy
```

### pick_drop_pattern
`pick_drop_pattern`提卸规则，该参数控制一辆车单趟的提货站点数和卸货站点数。有四个备选值:
* `SPMD`: 单提多卸
* `SPSD`: 单提单卸 (暂不支持)
* `MPMD`: 多提多卸 (暂不支持)
* `MPSD`: 多提单卸 (暂不支持)

其中，`SPMD`为默认值。demo如下所示:

```yaml
pick_drop_pattern: SPMD
```

### load_unload_policy
`load_unload_policy`装卸策略，该参数影响着订单在路由中的提卸顺序，有三个备选值:
* `FILO`: 先进后出(first input last output);
* `FIFO`: 先进先出(first input first output);
* `NONE`: 不限制;

其中，`FILO`为默认值。demo如下所示:

```yaml
load_unload_policy: FILO
```

### cargo_order_group_rule
`cargo_order_group_rule`订单的分组规则。缺省时则代表相同提货地、卸货地不做分组直接合并。
有两个参数：
* `label_code`: 订单label的编码，不可为空且不能使用不存在的label code;
* `label_value_group`: `label_code`对应`label_value`的组合, 不可为空且不能使用不存在的label code;

demo如下所示:

```yaml
cargo_order_group_rule:
  label_code: commodity
  label_value_group: [[A, B], [C, D]]
```

### hard_constraints

可配置的硬约束列表，填写指定的约束即代表约束作用于求解。每条约束包含两个字段`code`和`params`，
其中，`code`必须为已经在系统中定义的硬约束编码，`params`是字典类型描述硬约束的参数(按照约束的依赖控制是否必填)。
demo如下所示:

```yaml
hard_constraints:
  - code: MaxPickNodeCount
    params: {max_pick_node_count: 3}
  - code: MaxDropNodeCount
    params: {max_drop_node_count: 6}
```

目前支持的可配置约束如下：

#### MaxPickNodeCount

车次的最大提货节点数约束，可配置参数`max_pick_node_count`控制最大提货节点数。
demo如下所示:

```yaml
hard_constraints:
  - code: MaxPickNodeCount
    params: {max_pick_node_count: 3}
```

#### MaxDropNodeCount

车次的最大卸货节点数约束，可配置参数`max_drop_node_count`控制最大卸货节点数。
demo如下所示:

```yaml
hard_constraints:
  - code: MaxDropNodeCount
    params: {max_drop_node_count: 6}
```

#### LabelValueExclusive

标签值互斥约束：同一车次不能同时配送指定标签下互斥的标签值。可配置参数：

- `label_code`（必填）：标签编码，必须是 `Order` 或 `Location` 标签集合中已定义的标签。
- `entity`（必填）：约束作用的实体域，取值为 `Order` 或 `Location`。
- `groups`（必填）：二维字符串数组，每个子数组是一组互斥的标签值。
- `side`（可选，仅 `entity: Location`）：站点侧别，取值为 `Pick`、`Drop` 或 `Both`，默认 `Both`。
  `Pick` 只检查提货侧，`Drop` 只检查卸货侧，`Both` 两侧各检查一次；`entity: Order` 时不允许出现该参数。

`groups` 中同一组内的标签值两两不可同车；不同组之间不做传递合并，只有同时出现在某一组中的
值对才互斥。例如 `[[A, B, C], [B, D]]` 表示 `A/B/C` 两两不可同车、`B/D` 不可同车，但 `D`
与 `A`、`C` 可以同车。未出现在任何组中的标签值不受此约束限制。

```yaml
hard_constraints:
  - code: LabelValueExclusive
    params:
      label_code: temperature
      entity: Order
      groups: [[cold, ambient], [ambient, dangerous]]
```

#### LabelValueCompatible

标签值可拼约束：同一车次只能配送同一组内的标签值，不同组之间不可同车。可配置参数与
`LabelValueExclusive` 相同（`label_code`、`entity`、`groups`、`side`），但 `groups` 表示“可拼组”：

- 显式组必须两两不相交，同一标签值不能出现在多个显式组中；
- 同一标签下未出现在任何显式组中的标签值会汇总成一个默认组，默认组内的标签值仍可同车；
- 若标签的全部取值都已被显式组覆盖，则不会生成默认组。

例如 `[[A, B], [C, D]]` 表示 `A/B` 可同车、`C/D` 可同车，但两组之间不可同车；未列出的
`E/F` 会进入默认组，可彼此同车，但不能与 `A/B/C/D` 中的任意值同车。

```yaml
hard_constraints:
  - code: LabelValueCompatible
    params:
      label_code: region
      entity: Location
      side: Pick
      groups: [[north, south], [east, west]]
```

`entity: Location` 时，约束按 `side` 检查站点标签值集合：`Both` 分别检查提货侧与卸货侧，
两侧都通过才可行；`Pick` 只检查提货侧；`Drop` 只检查卸货侧；省略 `side` 时默认 `Both`。
`entity: Order` 时，约束检查车次所有订单的标签值并集，且不支持 `side` 参数。每条约束项只能选择
`LabelValueExclusive` 或 `LabelValueCompatible` 中的一个编码，即一条约束中互斥与可拼不会同时生效。

#### LabelValueMaxCount

标签值种类数上限约束：同一车次内指定标签出现过的不同取值个数不能超过 `max_count`。
可配置参数：

- `label_code`（必填）：标签编码，必须是 `Order` 或 `Location` 标签集合中已定义的标签。
- `entity`（必填）：约束作用的实体域，取值为 `Order` 或 `Location`。
- `max_count`（必填）：允许的标签值种类数上限，必须是非负整数；`0` 表示车次内不允许出现该标签的任何取值。
- `side`（可选，仅 `entity: Location`）：站点侧别，取值为 `Pick`、`Drop` 或 `Both`，默认 `Both`；
  `entity: Order` 时不允许出现该参数。

`entity: Order` 时统计车次全部订单的标签值并集；`entity: Location` 时按 `side` 统计站点标签值：

- `Pick` 只统计提货侧，`Drop` 只统计卸货侧；
- `Both`（含省略 `side`）把提货侧与卸货侧的取值**合并去重**后统计。注意这与
  `LabelValueExclusive` / `LabelValueCompatible` 的 `Both`（两侧各检查一次）不同：例如提货侧有
  `north/south`、卸货侧有 `south/east`，`Both` 口径下共 `north/south/east` 3 种。

```yaml
hard_constraints:
  - code: LabelValueMaxCount
    params: {label_code: temperature, entity: Order, max_count: 3}

  - code: LabelValueMaxCount
    params: {label_code: region, entity: Location, side: Pick, max_count: 2}

  - code: LabelValueMaxCount
    params: {label_code: region, entity: Location, max_count: 2}
```

### soft_constraints

可配置的软约束列表，填写指定的约束即代表约束作用于求解。每条约束包含两个字段`code`和`params`，
其中，`code`必须为已经在系统中定义的软约束编码，`params`是字典类型描述软约束的参数((按照约束的依赖控制是否必填))。
demo如下所示:

```yaml
soft_constraints:
  - code: Distance
    params: {dist_factor: 0.5}
```

#### Distance

总行驶距离的惩罚项，可配置参数`dist_factor` * 总行驶距离(m) 的值代表行驶距离惩罚。
demo如下所示:

```yaml
soft_constraints:
  - code: Distance
    params: {dist_factor: 0.5}
```
