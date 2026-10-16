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
* `SPSD`: 单提单卸
* `MPMD`: 多提多卸
* `MPSD`: 多提单卸

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
