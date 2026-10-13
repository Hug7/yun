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
`strategy_file`策略文件名，默认值 `strategy`。demo如下所示:

```yaml
strategy_file: strategy
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
`cargo_order_group_rule`有两个参数：
* `label_code`: 订单label的编码，不可为空且不能使用不存在的label code;
* `label_value_group`: `label_code`对应`label_value`的组合, 不可为空且不能使用不存在的label code;

demo如下所示:

```yaml
cargo_order_group_rule:
  label_code: commodity
  label_value_group: [[A, B], [C, D]]
```
