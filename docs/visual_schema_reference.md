# 可视化结果 JSON 接口文档

> 本文档描述 `resources/template/visual/` 下全部 JSON 文件的接口结构。字段完整性以 `problem/src/visual_plan_result.cpp` 的实际序列化逻辑为准；当前部分模板样本未展示 `LoadingDimensionCode`、`OrderLabels` 等字段，但生成文件会始终输出本文档列出的字段。

## 通用约定

- 每个文件的内容都是一个 JSON 数组，数组元素分别为车次、车次站点、订单明细或不可解订单记录。
- 字符串类型为 `string`，整数类型为 `int` 或 `long`，浮点类型为 `double`。
- 时间字符串格式为 `%Y-%m-%d %H:%M:%S`，时间戳为 Unix 时间戳，单位是秒。
- 所有字段均由序列化器固定输出。集合没有元素时输出 `[]`，字符串没有值时输出 `""`，数值没有值时输出 `0`。
- 编码字段引用相同规划请求中的场景数据，例如 `CarrierCode`、`VehicleModelCode`、`LocationCode`、`DimensionCode` 和 `LabelCode`。

## 文件总览

| 文件 | JSON 根结构 | 元素结构 | 说明 |
|---|---|---|---|
| `Loads.json` | `Load[]` | 车次对象 | 车次维度的装载、线路、成本、标签和计划时间 |
| `LoadLocations.json` | `LoadLocation[]` | 车次站点对象 | 车次线路中每个站点的作业、订单、标签和计划时间 |
| `UnassignedCargoOrders.json` | `CargoOrder[]` | 订单明细对象 | 未被指派到任何车次的订单明细 |
| `InfeasibleCargoOrders.json` | `InfeasibleCargoOrder[]` | 不可解订单对象 | 订单不可解原因，以及按承运商和车型区分的原因 |

## 复用对象

### DimensionObject（维度值对象）

用于 `VehicleLoadDimensions`、`LoadDimensions`、`LoadingRateDimensions`、`OrderDimensions`。

| 字段 | 数据类型 | 必填 | 说明 |
|---|---|---|---|
| DimensionCode | string | 是 | 维度编码，对应场景中的 `Dimension.Code` |
| DimensionValue | double | 是 | 该维度的数值 |

### LabelObject（标签对象）

用于 `PickLocationLabels`、`DropLocationLabels`、`LocationLabels`、`OrderLabels`。

| 字段 | 数据类型 | 必填 | 说明 |
|---|---|---|---|
| LabelCode | string | 是 | 标签编码，对应场景中的 `Label.Code` |
| LabelValues | string[] | 是 | 当前对象在该标签下的取值集合；没有取值时为空数组 |

### CargoOrderObject（订单明细对象）

`UnassignedCargoOrders.json` 的直接元素，以及 `LoadLocations.json[].CargoOrders` 的元素。

| 字段 | 数据类型 | 必填 | 说明 |
|---|---|---|---|
| CargoOrderCode | string | 是 | 父订单编码 |
| CargoSubOrderCode | string | 是 | 子订单编码 |
| Quantity | int | 是 | 子订单数量 |
| OrderDimensions | DimensionObject[] | 是 | 子订单的各维度值 |
| PickLocationCode | string | 是 | 提货站点编码 |
| DropLocationCode | string | 是 | 卸货站点编码 |
| OrderLabels | LabelObject[] | 是 | 子订单标签及标签值集合 |

---

## 1. `Loads.json`

根元素为 `Load[]`，每个元素描述一个车次。站点顺序、站点级订单明细和站点级时间窗见 `LoadLocations.json`。

| 字段 | 数据类型 | 必填 | 说明 |
|---|---|---|---|
| LoadCode | string | 是 | 车次编码；同一份结果内唯一 |
| CarrierCode | string | 是 | 承运商编码 |
| VehicleModelCode | string | 是 | 车型编码 |
| VehicleLoadDimensions | DimensionObject[] | 是 | 车辆在各维度上的装载能力 |
| LoadDimensions | DimensionObject[] | 是 | 车次在各维度上的峰值装载量 |
| LoadingRateDimensions | DimensionObject[] | 是 | 各维度的峰值装载率，峰值装载量除以车辆装载能力 |
| LoadingDimensionCode | string | 是 | 峰值装载率最大的维度编码 |
| LoadingRate | double | 是 | 最大峰值装载率 |
| TotalDrivingDistanceMeter | long | 是 | 总行驶距离，单位：米 |
| TotalDrivingTimeSecond | long | 是 | 总行驶时长，单位：秒 |
| TotalWorkTimeSecond | long | 是 | 总作业时长，单位：秒 |
| TotalWaitTimeSecond | long | 是 | 总等待时长，单位：秒 |
| TotalCostTimeSecond | long | 是 | 总成本时间，单位：秒 |
| SoftPenaltyValue | double | 是 | 软约束惩罚值 |
| CostValue | double | 是 | 成本值 |
| ObjectValue | double | 是 | 目标函数值 |
| PickLocationLabels | LabelObject[] | 是 | 车次所有提货站点的标签值集合 |
| DropLocationLabels | LabelObject[] | 是 | 车次所有卸货站点的标签值集合 |
| OrderLabels | LabelObject[] | 是 | 车次所有订单的标签值集合 |
| LocationSequences | string[] | 是 | 车次经过的站点编码顺序，不包含内部默认节点 |
| PlanEarliestStartTime | string | 是 | 最早计划开始时间 |
| PlanEarliestStartTimestamp | long | 是 | 最早计划开始时间戳，单位：秒 |
| PlanEarliestEndTime | string | 是 | 最早计划结束时间 |
| PlanEarliestEndTimestamp | long | 是 | 最早计划结束时间戳，单位：秒 |
| PlanLatestStartTime | string | 是 | 最晚计划开始时间 |
| PlanLatestStartTimestamp | long | 是 | 最晚计划开始时间戳，单位：秒 |
| PlanLatestEndTime | string | 是 | 最晚计划结束时间 |
| PlanLatestEndTimestamp | long | 是 | 最晚计划结束时间戳，单位：秒 |

### 关联关系

- `LoadCode` 与 `LoadLocations.json[].LoadCode` 对应，一个车次可对应多个站点记录。
- `CarrierCode`、`VehicleModelCode` 与该车次全部站点记录的对应字段一致。
- `LocationSequences` 应覆盖该车次站点记录中的 `LocationCode` 顺序。

---

## 2. `LoadLocations.json`

根元素为 `LoadLocation[]`，每个元素描述一个车次在线路上的一个站点。

| 字段 | 数据类型 | 必填 | 说明 |
|---|---|---|---|
| LoadCode | string | 是 | 所属车次编码 |
| CarrierCode | string | 是 | 承运商编码 |
| VehicleModelCode | string | 是 | 车型编码 |
| LocationCode | string | 是 | 当前站点编码 |
| ActivityType | string | 是 | 当前站点作业类型：`START`、`PICK`、`DROP`、`END` |
| CargoOrders | CargoOrderObject[] | 是 | 当前站点上装车或卸车的订单明细；无订单时为空数组 |
| LocationLabels | LabelObject[] | 是 | 当前站点的标签值集合 |
| OrderLabels | LabelObject[] | 是 | 当前站点涉及订单的标签值集合，取订单标签的并集 |
| PrevDrivingDistanceMeter | long | 是 | 从上一站点行驶到当前站点的距离，单位：米 |
| PrevDrivingTimeSecond | long | 是 | 从上一站点行驶到当前站点的时长，单位：秒 |
| WaitTimeSecond | long | 是 | 当前站点的等待时长，单位：秒 |
| WorkTimeSecond | long | 是 | 当前站点的作业时长，单位：秒 |
| PlanEarliestArriveTime | string | 是 | 最早计划到达时间 |
| PlanEarliestArriveTimestamp | long | 是 | 最早计划到达时间戳，单位：秒 |
| PlanEarliestStartWorkTime | string | 是 | 最早计划开始作业时间 |
| PlanEarliestStartWorkTimestamp | long | 是 | 最早计划开始作业时间戳，单位：秒 |
| PlanEarliestDepartTime | string | 是 | 最早计划离开时间 |
| PlanEarliestDepartTimestamp | long | 是 | 最早计划离开时间戳，单位：秒 |
| PlanLatestArriveTime | string | 是 | 最晚计划到达时间 |
| PlanLatestArriveTimestamp | long | 是 | 最晚计划到达时间戳，单位：秒 |
| PlanLatestStartWorkTime | string | 是 | 最晚计划开始作业时间 |
| PlanLatestStartWorkTimestamp | long | 是 | 最晚计划开始作业时间戳，单位：秒 |
| PlanLatestDepartTime | string | 是 | 最晚计划离开时间 |
| PlanLatestDepartTimestamp | long | 是 | 最晚计划离开时间戳，单位：秒 |

### 字段关系

- `PlanEarliestStartWorkTimestamp = PlanEarliestDepartTimestamp - WorkTimeSecond`。
- `PlanLatestStartWorkTimestamp = PlanLatestDepartTimestamp - WorkTimeSecond`。
- 若当前节点没有计划时间窗，时间字符串和数值时间戳保持为空字符串或 `0`。
- `PICK` 站点的 `CargoOrders` 表示装车明细，`DROP` 站点的 `CargoOrders` 表示卸货明细，`START`、`END` 站点通常没有订单。

---

## 3. `UnassignedCargoOrders.json`

根元素为 `CargoOrder[]`，每个元素直接使用 `CargoOrderObject`，表示未指派到任何车次的子订单明细。

| 字段 | 数据类型 | 必填 | 说明 |
|---|---|---|---|
| CargoOrderCode | string | 是 | 父订单编码 |
| CargoSubOrderCode | string | 是 | 子订单编码 |
| Quantity | int | 是 | 未指派数量 |
| OrderDimensions | DimensionObject[] | 是 | 子订单的各维度值 |
| PickLocationCode | string | 是 | 提货站点编码 |
| DropLocationCode | string | 是 | 卸货站点编码 |
| OrderLabels | LabelObject[] | 是 | 子订单标签及标签值集合 |

当所有订单均已指派时，文件内容为 `[]`。

---

## 4. `InfeasibleCargoOrders.json`

根元素为 `InfeasibleCargoOrder[]`，每个元素表示一组不可解订单及其原因。

| 字段 | 数据类型 | 必填 | 说明 |
|---|---|---|---|
| CargoOrderCodes | string[] | 是 | 不可解原因共同影响的父订单编码集合 |
| CommonInfeasibleReasons | InfeasibleReasonObject[] | 是 | 与具体车辆无关的不可解原因；没有时为空数组 |
| VehicleInfeasibleReasons | VehicleInfeasibleReasonObject[] | 是 | 按承运商和车型区分的不可解原因；没有时为空数组 |

### InfeasibleReasonObject（不可解原因对象）

| 字段 | 数据类型 | 必填 | 说明 |
|---|---|---|---|
| ReasonCode | string | 是 | 原因编码 |
| ConstraintCode | string | 是 | 触发该原因的约束编码 |
| MessageCN | string | 是 | 中文原因说明 |
| MessageEN | string | 是 | 英文原因说明 |

### VehicleInfeasibleReasonObject（车辆不可解原因对象）

| 字段 | 数据类型 | 必填 | 说明 |
|---|---|---|---|
| CarrierCode | string | 是 | 承运商编码 |
| VehicleModelCode | string | 是 | 车型编码 |
| InfeasibleReasons | InfeasibleReasonObject[] | 是 | 该承运商和车型组合下的不可解原因；没有时为空数组 |
