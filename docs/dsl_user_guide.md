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
