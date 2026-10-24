# 计费（运输成本）


## 1. 目标与范围

计费描述**一个车次（`Load`）的运输成本**，作为成本约束（cost constraint）参与
`obj_val = hard + soft + cost` 的比较。

- 输入：场景目录下的 4 张计费表 + 车次（车辆、线路、里程、节点、维度等）。
- 输出：该车次的一个运输成本（元），写入 `LoadConstrProfile::total_cost`。
- 不在本范围：税费、结算、发票、按订单拆分计费。

计费表是**可选输入**：场景目录不存在 `ChargeRule.csv` 时，计费功能关闭，成本恒为 0，
不影响既有用例。

## 2. 术语

| 术语 | 含义 |
|---|---|
| 计费编码 `ChargeCode` | 一个合同条款。一个车次可能命中多个条款，取**最便宜**的条款作为该车次运费 |
| 次级编码 `ChargeSecondaryCode` | 条款内部的计价分组。同一 OD 对下多条规则金额求和，不同 OD 组取**最大值**，不同次级编码再取**最大值** |
| 计费规则 `ChargeRule` | 一组查询键 + 一个阶梯条件 + 若干阶梯 + 若干计费项。规则是表里的行 |
| 阶梯 `ChargeTier` | 一个查询条件的取值区间 `[LowerBound, UpperBound)` |
| 计费项 `ChargeItem` | 阶梯内的一条计价公式 |
| 线路标签 `LocationLabelCode` | 用站点的哪一类标签描述线路，例如 `rate_zone` / `city` |
| 线路标签值 `ChargePathLabelValues` | 线路在 `LocationLabelCode` 上的取值序列，用 `-` 分隔，例如 `SH-SZ` |

## 3. 计费规则

### 3.1 计费对象

一次计费针对一个 `Load`。计算时读取：

- 车辆：`load->vehicle`（承运商、车型）；
- 线路：`ORDERED_ROUTE` 读取车次节点链上所有 `PICK` / `DROP` 节点（首尾 depot 哨兵不参与）；
- 订单 OD：`OD_PAIR` 读取车次上每个订单的提货站点与卸货站点；
- 里程：`load->get_total_travel_dist()`（米）；
- 提货节点数 / 卸货节点数：节点链上 `PICK` / `DROP` 节点的数量；
- 维度值：车次上所有订单的维度值之和（`total_load_dims`），不是峰值装载。

空车次、未分配车辆的车次成本为 0。

### 3.2 查询键

一条计费规则由下列字段定位：

| 字段 | 是否可空 | 匹配语义 |
|---|---|---|
| `ChargeCode` | 否 | 合同条款编码 |
| `ChargeSecondaryCode` | 否 | 条款内计价分组 |
| `CarrierCode` | 是 | 空 = 任意承运商；非空 = 必须等于车次车辆承运商 |
| `VehicleModelCode` | 是 | 空 = 任意车型；非空 = 必须等于车次车型 |
| `ChargePathType` | 是 | 空 = 不限制线路；`OD_PAIR` / `ORDERED_ROUTE` 见 3.3 |
| `LocationLabelCode` | 是 | 线路匹配使用的站点标签；`ChargePathType` 非空时必填 |
| `ChargePathLabelValues` | 是 | 线路标签值序列；`ChargePathType` 非空时必填 |

空字段是**通配符**，不是“必须为空”。例如只填 `CarrierCode` 的规则对所有线路生效。

### 3.3 线路匹配

`ChargePathType` 决定使用哪一组站点做匹配：

#### `OD_PAIR`：按订单的提货站点 / 卸货站点

- **不取车次的首末停靠点**；
- 对车次上的每个订单，读取 `Order::pick_loc` / `Order::drop_loc` 在
  `LocationLabelCode` 下的标签值；
- 车次的订单 OD 集合 = 所有订单的 `(提货标签值, 卸货标签值)` 笛卡尔积去重后的集合。
  例如订单 1 是 `SH-SZ`，订单 2 是 `SH-HZ`，则 OD 集合为 `{SH-SZ, SH-HZ}`；
- 同一站点在该标签下有多个值时，会展开成多个 OD 对；单个车次最多展开 4096 个 OD 对，
  超过时该标签的 OD 计费整体失效；
- 典型值：`SH-SZ`，表示订单从 SH 提货、送到 SZ。

**同一 `ChargeCode` + `ChargeSecondaryCode` 的覆盖要求**：

- 只要该次级编码下存在 OD 规则，它就是一个“按 OD 计价”的次级编码；
- 车次 OD 集合中的**每一个** OD 对，都必须在该次级编码下至少有一条命中的 OD 规则
  （即“有报价”）；
- 缺任意一个 OD 对的报价，**整个次级编码不可计费**，而不是只跳过缺失的那一组；
- 全部覆盖后，同一 OD 对命中规则的金额先求和，再对所有 OD 对取**最大值**，作为该次级
  编码的 OD 金额；即“不同 OD 组不求和，取最贵的报价”；同一 OD 对有多条规则时，这些规则
  金额仍然相加；
- 同一 OD 对在车次中出现多次（多个订单）时，只算一次（按 OD 组计价，不按订单数重复）；
- 同一次级编码下的所有 OD 规则必须使用同一个 `LocationLabelCode`，否则读取失败；
- 同一次级编码下如果同时有 `ORDERED_ROUTE` 或无线路（`ChargePathType` 为空）规则，
  它们不参与 OD 覆盖判断，但在 OD 覆盖通过后其金额也计入该次级编码。

> 如果业务希望“同一 OD 的多个订单按订单数重复计费”，或希望缺一个 OD 报价时只跳过该
> OD 而不是整条次级编码失效，需要调整 `ChargeManager::evaluate` 的覆盖/求和逻辑。

#### `ORDERED_ROUTE`：按车次停靠点序列

1. 只取 `PICK` / `DROP` 节点，按节点链顺序；
2. 每个节点读取该标签下的全部标签值（一个站点可以有多个值）；
3. **合并连续相同的标签值集合**。例如 `SH, SH, SZ` 合并为 `SH, SZ`；
4. 合并后的序列长度必须等于标签值个数，且逐位匹配。典型值：`SH-SZ-WX`。

#### 空

不做线路匹配，但仍然可以走阶梯条件。

“包含”指该站点在该标签下有这个值；站点没有该标签时视为不匹配。

### 3.4 阶梯

一条规则的阶梯条件**只能是一种**，在规则上声明，作用于该规则的所有阶梯：

| `TierConditionType` | 含义 | 表内单位 | 内部比较值 |
|---|---|---|---|
| 空 | 无查询条件 | — | 只有一个阶梯 |
| `DISTANCE` | 车次总里程 | km（可小数） | 米 |
| `PICK_NODE_COUNT` | 提货节点数 | 个 | 整数 |
| `DROP_NODE_COUNT` | 卸货节点数 | 个 | 整数 |
| `DIMENSION` | 指定维度的总量 | 维度自然单位（可小数） | 维度原始值（×10^precision） |

- 区间为左闭右开 `[LowerBound, UpperBound)`；`UpperBound` 为空表示正无穷。
- 阶梯按 `LowerBound` 升序排列，区间不允许重叠；允许留空档（空档内该规则不生效）。
- 无查询条件时，规则**有且仅有一个**阶梯，区间固定为 `[0, +∞)`，表内上下界可留空。
- 条件为 `DIMENSION` 时，规则必须指定 `TierDimensionCode`。

### 3.5 计费项

阶梯内可有多条计费项，金额相加。`Price` 的单位是**元/计价单位**，最多 4 位小数。

| `ItemType` | 公式 | `DimensionCode` |
|---|---|---|
| `FLAT` | `Price` | 必须为空 |
| `DISTANCE` | `Price × 里程(km)` | 必须为空 |
| `PICK_STOP` | `Price × max(0, 提货节点数 − 1)` | 必须为空 |
| `DROP_STOP` | `Price × max(0, 卸货节点数 − 1)` | 必须为空 |
| `DIMENSION` | `Price × 维度总量(自然单位)` | 必填 |
| `DIMENSION_DISTANCE` | `Price × 维度总量(自然单位) × 里程(km)` | 必填 |

说明：

- `PICK_STOP` / `DROP_STOP` 是“串点费”，**第一个点免费，超出的点收费**。
- 维度总量是该维度在所有订单上的数量之和；自然单位 = 原始值 ÷ 10^precision。
- 所有金额内部用 `1e-4 元` 的整数（万分之一元）计算，最后 ÷10000 转为元；
  每一步乘法用 128 位中间值，除法四舍五入，避免浮点误差和溢出。

### 3.6 汇总

对一个车次：

1. 对每条命中的规则，按 3.4 选出阶梯，按 3.5 算出规则金额；
2. 对每个 `(ChargeCode, ChargeSecondaryCode)`：
   - 如果它含 OD 规则，先检查车次 OD 集合是否全部有报价；缺任意一个则整个次级编码跳过；
   - 通过覆盖检查后，OD 规则按 OD 对分组：组内多条规则金额求和，再对各组取最大值；
     非 OD 规则（`ORDERED_ROUTE` / 无线路）金额单独求和后加到该最大值上；
3. 同一 `ChargeCode` 下不同次级编码的汇总值取**最大值**，得到该条款金额；
4. 所有命中且通过覆盖检查的 `ChargeCode` 中取**最小值**，作为车次运费；
5. 没有任何可计费条款时，返回 `matched=false`；`CcCharge` 将该车次标记为不可行，
   原因是 `NO_CHARGE_RATE`。

示例：车次含 `SH-SZ`、`SH-HZ` 两组 OD，同一 `ChargeCode + ChargeSecondaryCode` 下
`SH-SZ` 报价 100 元、`SH-HZ` 报价 80 元，则该次级编码金额取较贵的 100 元；如果
`SH-HZ` 没有报价，则该次级编码整体不可用。

即：

```text
cost(load) = min over charge_code (
                 max over secondary_code (
                     sum over matched rules of rule_amount
                 )
             )
```

### 3.7 单位与精度

| 量 | 表内单位 | 内部单位 |
|---|---|---|
| 里程 | km，可小数 | 米（long） |
| 节点数 | 个 | 个（int） |
| 维度 | 自然单位，可小数 | 原始值（long，×10^precision） |
| 金额 | 元/单位，最多 4 位小数 | 1e-4 元（long） |
| 最终成本 | 元 | double（÷10000） |

维度精度必须在 `[0, 9]`，否则读取失败。

### 3.8 边界与异常

- `ChargeCode` / `ChargeSecondaryCode` / `RuleCode` / `TierSeq` 不能为空或重复。
- `ChargePathType` 非空时，`LocationLabelCode`、`ChargePathLabelValues` 必填；
  `OD_PAIR` 必须恰好 2 个值，`ORDERED_ROUTE` 至少 1 个值。
- `ChargePathType` 为空时，`LocationLabelCode`、`ChargePathLabelValues` 必须为空。
- 条件不是 `DIMENSION` 时，`TierDimensionCode` 必须为空。
- 规则必须有至少一个阶梯；每个阶梯必须有至少一个计费项。
- 同一规则内阶梯区间不能重叠；无条件的规则只能有一个阶梯。
- 引用的承运商、车型、标签、标签值、维度、计费编码必须存在。
- 两条规则的查询键完全相同（含线路和条件）时视为数据错误，读取失败，避免重复计费。
- 命中规则但阶梯有空档、线路不匹配、维度不存在时，该规则跳过，不报错。
- 计费表存在但某车次无任何可计费条款时，`CcCharge` 标记不可行（`NO_CHARGE_RATE`）。

## 4. 表设计

所有表放在场景目录（与 `CargoOrder.csv` 同级），UTF-8，首行是表头。

### 4.1 `ChargeCode.csv`（计费编码 / 合同条款）

| 列 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `Code` | string | 是 | 计费编码，唯一 |
| `Name` | string | 否 | 名称，仅用于日志/排查 |

```csv
Code,Name
TC001,华东标准运费
TC002,华东经济运费
```

### 4.2 `ChargeRule.csv`（计费规则 / 查询键）

| 列 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `RuleCode` | string | 是 | 规则唯一编码，供阶梯/计费项引用 |
| `ChargeCode` | string | 是 | 引用 `ChargeCode.Code` |
| `ChargeSecondaryCode` | string | 是 | 次级编码 |
| `CarrierCode` | string | 否 | 空 = 任意 |
| `VehicleModelCode` | string | 否 | 空 = 任意 |
| `ChargePathType` | enum | 否 | `OD_PAIR` / `ORDERED_ROUTE` / 空 |
| `LocationLabelCode` | string | 条件 | 线路标签编码 |
| `ChargePathLabelValues` | string | 条件 | 用 `-` 分隔的标签值序列 |
| `TierConditionType` | enum | 否 | `DISTANCE` / `PICK_NODE_COUNT` / `DROP_NODE_COUNT` / `DIMENSION` / 空 |
| `TierDimensionCode` | string | 条件 | `TierConditionType=DIMENSION` 时必填 |

```csv
RuleCode,ChargeCode,ChargeSecondaryCode,CarrierCode,VehicleModelCode,ChargePathType,LocationLabelCode,ChargePathLabelValues,TierConditionType,TierDimensionCode
R001,TC001,BASE,AP,AP_7M2,OD_PAIR,rate_zone,SH-SZ,DISTANCE,
R002,TC001,FUEL,AP,AP_7M2,OD_PAIR,rate_zone,SH-SZ,DISTANCE,
R003,TC002,BASE,,,,,,,DISTANCE,
R004,TC002,WEIGHT,,,,,,DIMENSION,weight
```

### 4.3 `ChargeTier.csv`（阶梯）

| 列 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `RuleCode` | string | 是 | 引用 `ChargeRule.RuleCode` |
| `TierSeq` | int | 是 | 规则内唯一，从 1 开始 |
| `LowerBound` | decimal | 否 | 下界（含）；空 = 0 |
| `UpperBound` | decimal | 否 | 上界（不含）；空 = +∞ |

> 上下界的单位由规则的 `TierConditionType` 决定：`DISTANCE` 用 km，
> `DIMENSION` 用维度自然单位，节点数用整数。

```csv
RuleCode,TierSeq,LowerBound,UpperBound
R001,1,0,100
R001,2,100,
R003,1,0,50
R003,2,50,200
R003,3,200,
R004,1,0,10
R004,2,10,
```

### 4.4 `ChargeItem.csv`（计费项）

| 列 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `RuleCode` | string | 是 | 引用 `ChargeRule.RuleCode` |
| `TierSeq` | int | 是 | 引用 `ChargeTier.TierSeq` |
| `ItemType` | enum | 是 | 见 3.5 |
| `Price` | decimal | 是 | 元/单位，最多 4 位小数，≥ 0 |
| `DimensionCode` | string | 条件 | `DIMENSION` / `DIMENSION_DISTANCE` 时必填 |

```csv
RuleCode,TierSeq,ItemType,Price,DimensionCode
R001,1,FLAT,500.0000,
R001,1,DISTANCE,2.3500,
R002,1,DISTANCE,0.3000,
R003,1,FLAT,300.0000,
R003,2,FLAT,280.0000,
R003,3,FLAT,260.0000,
R004,1,DIMENSION,0.5000,weight
```

## 5. 读取与校验

读取顺序（`ChargeCsvReader`）：

1. `ChargeCode.csv`：建立 `code -> charge_code_ind`，索引连续；
2. `ChargeRule.csv`：把承运商、车型、标签、标签值、维度解析成整数索引；
3. `ChargeTier.csv`：按规则分组，解析区间并校验；
4. `ChargeItem.csv`：挂到 `(RuleCode, TierSeq)`，校验计费项；
5. 阶梯校验：同一 `RuleCode` 内先按 `LowerBound` 排序，检查相邻区间是否重叠；
   重叠直接抛错，错误信息带两行行号、`RuleCode` 和 `TierSeq`；
6. 收口：阶梯按 `LowerBound` 排序、拍平成连续数组，建立运行时索引。

读取期允许使用 `unordered_map` 做“字符串 → 索引”的冷路径解析；**计算热路径不再使用 map**。

`ChargeManager::build_indexes` 建立：

- `path_label_inds`：所有线路规则用到的标签槽位；
- `loc_label_offsets` / `loc_label_values`：`location_ind × label_slot` 的 CSR，
  把站点标签值预先摊平，避免热路径查 `unordered_map`；
- `vehicle_rule_offsets` / `vehicle_rule_inds`：按车辆索引预筛“承运商/车型”规则；
- `generic_rule_inds`：不限定承运商/车型的规则；
- `dim_scales`：维度的 `10^precision`。

## 6. 计算流程

```text
eval(load):
  if 计费关闭 or 空车次 or 无车辆: return 0
  按需读取：订单 OD、停靠点、里程、节点数、维度（只取规则真正需要的字段）
  按订单计算 OD 对集合（笛卡尔积去重）
  按停靠点计算 ORDERED 标签序列（连续相同集合合并）
  for rule in generic_rules + rules_of_vehicle:
      if rule 是 OD_PAIR:
          规则 OD 对不在车次 OD 集合中 -> continue
          标记该 OD 对在 rule.secondary 下已覆盖
      if rule 是 ORDERED_ROUTE and 停靠点序列不匹配: continue
      metric = 按 rule.condition_type 取里程/节点数/维度
      tier = 在 rule 的阶梯中找 [lower, upper) 包含 metric 的阶梯
      if 没有阶梯: continue
      amount = sum(rule 的 tier 内所有 item 的公式)
      if rule 是 OD_PAIR: 累加到 secondary × OD 对 的桶
      else: 累加到 secondary 的非 OD 桶
  for secondary:
      if secondary 含 OD 规则:
          if 车次 OD 集合未全部覆盖: skip
          amount = max(各 OD 对的桶值) + 非 OD 桶
      else:
          amount = 非 OD 桶
      用 amount 更新 charge_code 最大值
  cost = min over charge_code ( max over secondary (桶值) )
  if 没有可计费条款: return {amount=0, matched=false}
  return {amount=cost, matched=true}
  # CcCharge 根据 matched=false 返回 infeasible(NO_CHARGE_RATE)
```

## 7. 性能设计

计费在每个 `Load` 每次 `eval_load` 时都会执行。实现约束：

1. **热路径零 `unordered_map` / 零字符串比较**：全部用 `int` 索引；
2. **连续内存**：规则、阶梯、计费项各用一条 `std::vector`，阶梯/计费项用
   `begin + len` 引用，避免指针追逐和每规则小分配；
3. **按车辆预筛**：`Vehicle::ind` 直接索引，把承运商/车型不匹配的规则提前排除；
4. **站点标签值 CSR**：线路匹配不再逐站查标签 map；
5. **线路签名只算一次**：OD 按订单算一次 OD 对集合，ORDERED 按停靠点算一次序列，
   规则匹配只做 `int` / `uint64_t` 比较；
6. **聚合用扁平数组 + epoch**：`secondary` 桶和 `charge_code` 最大值用连续数组，
   用 epoch 标记本轮是否写过，避免每轮 `clear` 整张表；
7. **整数定点**：金额用 `1e-4 元` 的 `long`，乘法用 `__int128` 中间值，
   最后一次性转 `double`；
8. **线程本地 scratch**：并行求解时每个工作线程各用一份 `thread_local` 序列/桶，
   不在 `ChargeManager` 上放可变状态，保证只读共享、无锁；
9. **按需加载**：`ChargeLoadView` 是懒加载接口，`ChargeManager` 只在规则真正需要时调用
   对应 getter：
   - 没有 OD 规则，不读取订单提货/卸货站点；
   - 没有 `ORDERED_ROUTE` 规则，不读取车次停靠点；
   - 没有里程条件/计费项，不读取总里程；
   - 没有维度条件/计费项，不读取维度总量；
   - 没有提货/卸货节点条件或串点项，不统计节点数；
10. **求值流程拆分**：`ChargeEvaluator` 把“线路匹配 / 阶梯匹配 / 计费项 / 汇总”拆成
    普通成员函数，便于 review 和单测；没有虚函数、没有堆分配，编译器可在同一
    translation unit 内内联。

复杂度：`O(R_v + S × L)`，其中 `R_v` 是该车辆可见的规则数，`S` 是停靠点数，
`L` 是计费线路标签种类数（通常为 1）。不随订单数、维度数二次增长。

## 8. 示例

场景：`R001` 命中，里程 120 km，提货 2 点、卸货 2 点，重量 12.5 t。

- 阶梯：`[100, +∞)`；
- 计费项：`DISTANCE 2.35 元/km` + `PICK_STOP 50 元/点` + `DROP_STOP 50 元/点`；
- 金额 = `2.35 × 120 + 50 × (2−1) + 50 × (2−1) = 282 + 50 + 50 = 382 元`。

## 9. 假设

以下为计费功能的默认口径：

1. 阶梯条件“只能选一条”按**每条规则一个条件类型**实现（同一规则的阶梯共享条件）；
2. 串点费按“超出第一个点”计费；
3. 无任何可计费条款时标记不可行，原因是 `NO_CHARGE_RATE`；
4. `DISTANCE` 阶梯上下界用 km，计费项单价用元/km；
5. 连续相同标签值集合会合并，避免同一城市多个站点导致线路无法匹配；
6. 金额按万分之一元定点，最终四舍五入到 0.0001 元；
7. `ChargePathLabelValues` 对 `OD_PAIR` 是“提货标签值-卸货标签值”两个值，对
   `ORDERED_ROUTE` 是整条线路的标签值序列；
8. 多订单按**不同 OD 组**分别报价后取**最大值**（不求和）；同一 OD 组的多个订单只算一次；
9. OD 次级编码要求车次 OD 集合全部有报价；缺任意一个则整个次级编码不可用；
10. 同一 `ChargeCode + ChargeSecondaryCode` 下的 OD 规则必须使用同一
    `LocationLabelCode`；

## 10. 实现位置与验证

| 内容 | 文件 |
|---|---|
| 领域模型、索引与计算 | `scenario/include/bdm_charge.h`、`scenario/src/bdm_charge.cpp` |
| CSV 读取与校验 | `scenario/include/scr_charge_reader.h`、`scenario/src/scr_charge_reader.cpp`、`scenario/src/scr_charge_reader_tier.cpp`、`../scenario/include/scr_charge_reader_internal.h`、`scenario/src/scr_charge.cpp` |
| 成本约束 | `problem/include/cc_charge.h`、`problem/src/cc_charge.cpp` |
| 车次计费视图 | `scenario/include/bdm_charge.h`（懒加载接口）、`problem/src/cc_charge.cpp`（Load 适配器） |
| 求值流程 | `scenario/src/bdm_charge_eval.cpp`（`ChargeEvaluator`：线路匹配、阶梯匹配、计费项、汇总） |
| 车次维度总量缓存 | `problem/include/pdm_load_profile.h`、`problem/src/pdm_load_common.cpp` |
| 表模板 | `resources/template/scenario/Charge*.csv` |
| 单元测试 | `tests/scenario/charge_manager_test.cpp`、`tests/data/charge_case/` |

验证方式：

```bash
cmake --build cmake-build-debug -j8
./cmake-build-debug/tests/charge_manager_test
```

测试覆盖：OD 线路、有序线路、里程阶梯、维度阶梯、次级编码求和取最大、计费编码取最小、
串点费、维度×里程、连续站点合并、订单 OD 与首末站点区分、多 OD 覆盖与取最贵报价、
部分次级编码因缺 OD 报价被跳过、某 OD 组只有阶梯空档时整条次级编码不可用、
阶梯区间重叠被拒绝、懒加载只取必要字段、无匹配费率返回 matched=false、
线路不匹配、CSV 读取全链路。

懒加载验证：`LazyLoadsOnlyRequiredFields` 用计数视图确认纯 `FLAT` 规则只访问车辆索引，
不触发里程、节点数、维度、停靠点、订单等任何 getter。

端到端验证：给 `case01` 增加一条 `FLAT 500 元 + 1 元/km` 的通用规则后，
`PlanSummary.TotalCost` 与 `Loads.json` 中每个 `CostValue` 均等于
`500 + 里程(km) × 1`，说明计费已接入 `Problem::eval_load` 并进入目标函数。
