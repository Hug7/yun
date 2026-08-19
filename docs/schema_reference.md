# 数据表字段及依赖关系文档

> **约定**: 数据类型和取值范围由后续补充，本文档仅提供表结构、字段说明、主键、外键依赖关系。

---

## 表分组总览

| 分组 | 表名 | CSV 文件 | 简述 |
|---|---|---|---|
| 基础字典 | Dimension | Dimension.csv | 计算维度 |
| 基础字典 | Label | Label.csv | 标签集合 |
| 基础字典 | LabelValue | LabelValue.csv | 标签可选值 |
| 基础字典 | LabelApply | LabelApply.csv | 标签和实体的映射 |
| 站点 | Location | Location.csv | 站点基础信息 |
| 站点 | LocationLabelValue | LocationLabelValue.csv | 站点的标签值 |
| 站点 | LocationAvailableVehicle | LocationAvailableVehicle.csv | 站点可用车辆 |
| 站点 | WorkCalendarDaily | WorkCalendarDaily.csv | 工作日历（按天） |
| 站点 | WorkCalendarWeekly | WorkCalendarWeekly.csv | 工作日历（按周） |
| 站点 | WorkFixed | WorkFixed.csv | 站点固定作业时间 |
| 站点 | WorkEffect | WorkEffect.csv | 站点作业效率 |
| 车型 | VehicleModel | VehicleModel.csv | 车型定义 |
| 车型 | VehicleModelDimensionValue | VehicleModelDimensionValue.csv | 车型维度值 |
| 车型 | VehicleModelLabelValue | VehicleModelLabelValue.csv | 车型标签值 |
| 承运商 | Carrier | Carrier.csv | 承运商 |
| 承运商 | CarrierLabelValue | CarrierLabelValue.csv | 承运商标签值 |
| 车辆 | Vehicle | Vehicle.csv | 车辆 |
| 路由网络 | DistMatrixCode | DistMatrixCode.csv | 距离矩阵编码 |
| 路由网络 | DistMatrix | DistMatrix.csv | 距离矩阵 |
| 订单 | CargoOrder | CargoOrder.csv | 订单 |
| 订单 | CargoSubOrder | CargoSubOrder.csv | 子订单 |
| 订单 | CargoSubOrderDimensionValue | CargoSubOrderDimensionValue.csv | 子订单维度值 |
| 订单 | CargoSubOrderLabelValue | CargoSubOrderLabelValue.csv | 子订单标签值 |

---

## 基础字典

### 1. Dimension（计算维度）

- **CSV 文件**: `Dimension.csv`
- **主键**: `Code`

| # | 字段 | 数据类型 | 必填 | 说明 | 可选值 / 取值范围 |
|---|---|---|---|---|---|
| 1 | Code | string | 是 | 维度编码，唯一标识 | |
| 2 | Name | string | 否 | 维度名称 | |
| 3 | Precision | int | 是 | 精度，0=整数，1=小数点后1位...计算过程中为了保证每个维度都是整数，会按照精度对数值放大并直接截断取整 | [0, 4] |

**外键依赖**: 无

**被依赖**:
| 依赖方表 | 依赖方字段 | 关系 |
|---|---|---|
| VehicleModelDimensionValue | DimensionCode | 1 : N |
| CargoSubOrderDimensionValue | DimensionCode | 1 : N |
| WorkEffect | DimensionCode | 1 : N |

---

### 2. Label（标签）

- **CSV 文件**: `Label.csv`
- **主键**: `Code`

| # | 字段名 | 数据类型 | 必填 | 说明 | 可选值 / 取值范围 |
|---|---|---|---|---|---|
| 1 | Code | string | 是 | 标签编码，唯一标识 | |
| 2 | Name | string | 否 | 标签名称 | |

**外键依赖**: 无

**被依赖**:
| 依赖方表 | 依赖方字段 | 关系 |
|---|---|---|
| LabelValue | LabelCode | 1 : N |
| LabelApply | LabelCode | 1 : N |
| LocationLabelValue | LabelCode | 1 : N |
| VehicleModelLabelValue | LabelCode | 1 : N |
| CarrierLabelValue | LabelCode | 1 : N |
| CargoSubOrderLabelValue | LabelCode | 1 : N |

---

### 3. LabelValue（标签可选值）

- **CSV 文件**: `LabelValue.csv`
- **主键**: (`LabelCode`, `LabelValue`)

| # | 字段名 | 数据类型 | 必填 | 说明 | 可选值 / 取值范围 |
|---|---|---|---|---|---|
| 1 | LabelCode | string | 是 | 标签编码 | 必须存在于 Label.Code |
| 2 | LabelValue | string | 是 | 标签取值 | |

**外键依赖**:
| 本表字段 | 引用表 | 引用字段 |
|---|---|---|
| LabelCode | Label | Code |

**被依赖**:
| 依赖方表 | 依赖方字段 | 关系 |
|---|---|---|
| LocationLabelValue | (LabelCode, LabelValue) | 1 : N |
| VehicleModelLabelValue | (LabelCode, LabelValue) | 1 : N |
| CarrierLabelValue | (LabelCode, LabelValue) | 1 : N |
| CargoSubOrderLabelValue | (LabelCode, LabelValue) | 1 : N |

---

### 4. LabelApply（标签和实体的映射）

- **CSV 文件**: `LabelApply.csv`
- **主键**: (`LabelCode`, `ApplyItem`)

| # | 字段名 | 数据类型 | 必填 | 说明 | 可选值 / 取值范围 |
|---|---|---|---|---|---|
| 1 | LabelCode | string | 是 | 标签编码 | 必须存在于 Label.Code |
| 2 | ApplyItem | string | 是 | 适用实体类型 | Location / VehicleModel / Order / Carrier |

**外键依赖**:
| 本表字段 | 引用表 | 引用字段 |
|---|---|---|
| LabelCode | Label | Code |

**被依赖**: 无

---

## 站点

### 5. Location（站点）

- **CSV 文件**: `Location.csv`
- **主键**: `Code`

| # | 字段名 | 数据类型 | 必填 | 说明 | 可选值 / 取值范围 |
|---|---|---|---|---|---|
| 1 | Code | string | 是 | 站点编码，唯一标识 | |
| 2 | Name | string | 否 | 站点名称 | |
| 3 | Lng | double | 是 | 经度 | |
| 4 | Lat | double | 是 | 纬度 | |

**外键依赖**: 无

**被依赖**:
| 依赖方表 | 依赖方字段 | 关系 |
|---|---|---|
| LocationLabelValue | LocationCode | 1 : N |
| WorkCalendarDaily | LocationCode | 1 : N |
| WorkCalendarWeekly | LocationCode | 1 : N |
| WorkFixed | LocationCode | 1 : 1 |
| WorkEffect | LocationCode | 1 : N |
| LocationAvailableVehicle | LocationCode | 1 : N |
| DistMatrix | FromLocationCode | 1 : N |
| DistMatrix | ToLocationCode | 1 : N |
| Vehicle | OriginLocationCode | 1 : N |
| Vehicle | DestinationLocationCode | 1 : N |
| CargoOrder | PickLocationCode | 1 : N |
| CargoOrder | DropLocationCode | 1 : N |

---

### 6. LocationLabelValue（站点标签值）

- **CSV 文件**: `LocationLabelValue.csv`
- **主键**: (`LocationCode`, `LabelCode`)

| # | 字段名 | 数据类型 | 必填 | 说明 | 可选值 / 取值范围 |
|---|---|---|---|---|---|
| 1 | LocationCode | string | 是 | 站点编码 | 必须存在于 Location.Code |
| 2 | LabelCode | string | 是 | 标签编码 | 必须存在于 Label.Code, 并且存在LabelApply.LabelCode=LabelCode & LabelApply.ApplyItem=`Location` |
| 3 | LabelValue | string | 是 | 标签取值 | 必须存在于 LabelValue(LabelCode, LabelValue) |

**外键依赖**:
| 本表字段 | 引用表 | 引用字段 |
|---|---|---|
| LocationCode | Location | Code |
| (LabelCode, LabelValue) | LabelValue | (LabelCode, LabelValue) |
| LabelCode | LabelApply | (LabelCode, ApplyItem=`Location`) |

**被依赖**: 无

---

### 7. LocationAvailableVehicle（站点可用车辆）

- **CSV 文件**: `LocationAvailableVehicle.csv`
- **主键**: (`LocationCode`, `CarrierCode`, `VehicleModelCode`)

| # | 字段名 | 数据类型 | 必填 | 说明 | 可选值 / 取值范围 |
|---|---|---|---|---|---|
| 1 | LocationCode | string | 是 | 站点编码 | 必须存在于 Location.Code |
| 2 | CarrierCode | string | 是 | 承运商编码 | 必须存在于 Carrier.Code |
| 3 | VehicleModelCode | string | 是 | 车型编码 | 必须存在于 VehicleModel.Code；同时 (CarrierCode, VehicleModelCode) 必须存在于 Vehicle |

**外键依赖**:
| 本表字段 | 引用表 | 引用字段 |
|---|---|---|
| LocationCode | Location | Code |
| CarrierCode | Carrier | Code |
| VehicleModelCode | VehicleModel | Code |

**被依赖**: 无

---

### 8. WorkCalendarDaily（工作日历-按天）

- **CSV 文件**: `WorkCalendarDaily.csv`
- **主键**: (`LocationCode`, `CalendarType`)

| # | 字段名 | 数据类型 | 必填 | 说明 | 可选值 / 取值范围 |
|---|---|---|---|---|---|
| 1 | LocationCode | string | 是 | 站点编码 | 必须存在于 Location.Code |
| 2 | CalendarType | string | 是 | 日历类型 | PICK / DROP / RESTRICT |
| 3 | Daily | string | 是 | 每天的时间窗列表, 格式应遵循`%H%M%H%M;%H%M%H%M`, 其中`%H%M%H%M`是一段时间窗使用 `";"` 作为分割符 |  |

**外键依赖**:
| 本表字段 | 引用表 | 引用字段 |
|---|---|---|
| LocationCode | Location | Code |
**被依赖**: 无

**互斥**: 和`WorkCalendarWeekly`互斥, 相同主键的只能存在一个

---

### 9. WorkCalendarWeekly（工作日历-按周）

- **CSV 文件**: `WorkCalendarWeekly.csv`
- **主键**: (`LocationCode`, `CalendarType`)

| # | 字段名 | 数据类型 | 必填 | 说明 | 可选值 / 取值范围 |
|---|---|---|---|---|---|
| 1 | LocationCode | string | 是 | 站点编码 | 必须存在于 Location.Code |
| 2 | CalendarType | string | 是 | 日历类型 | PICK / DROP / RESTRICT |
| 3 | Monday | string | 否 | 周一的时间窗列表, 格式应遵循`%H%M%H%M;%H%M%H%M`, 其中`%H%M%H%M`是一段时间窗使用 `";"` 作为分割符, 为空时则不限制 | |
| 4 | Tuesday | string | 否 | 周二的时间窗列表, 格式应遵循`%H%M%H%M;%H%M%H%M`, 其中`%H%M%H%M`是一段时间窗使用 `";"` 作为分割符, 为空时则不限制 | |
| 5 | Wednesday | string | 否 | 周三的时间窗列表, 格式应遵循`%H%M%H%M;%H%M%H%M`, 其中`%H%M%H%M`是一段时间窗使用 `";"` 作为分割符, 为空时则不限制 | |
| 6 | Thursday | string | 否 | 周四的时间窗列表, 格式应遵循`%H%M%H%M;%H%M%H%M`, 其中`%H%M%H%M`是一段时间窗使用 `";"` 作为分割符, 为空时则不限制 | |
| 7 | Friday | string | 否 | 周五的时间窗列表, 格式应遵循`%H%M%H%M;%H%M%H%M`, 其中`%H%M%H%M`是一段时间窗使用 `";"` 作为分割符, 为空时则不限制 | |
| 8 | Saturday | string | 否 | 周六的时间窗列表, 格式应遵循`%H%M%H%M;%H%M%H%M`, 其中`%H%M%H%M`是一段时间窗使用 `";"` 作为分割符, 为空时则不限制 | |
| 9 | Sunday | string | 否 | 周日的时间窗列表, 格式应遵循`%H%M%H%M;%H%M%H%M`, 其中`%H%M%H%M`是一段时间窗使用 `";"` 作为分割符, 为空时则不限制 | |

**外键依赖**:
| 本表字段 | 引用表 | 引用字段 |
|---|---|---|
| LocationCode | Location | Code |

**被依赖**: 无

**互斥**: 和`WorkCalendarDaily`互斥，相同主键的只能存在一个

---

### 10. WorkFixed（站点固定作业时间）

- **CSV 文件**: `WorkFixed.csv`
- **主键**: `LocationCode`

| # | 字段名 | 数据类型 | 必填 | 说明 | 可选值 / 取值范围 |
|---|---|---|---|---|---|
| 1 | LocationCode | string | 是 | 站点编码 | 必须存在于 Location.Code |
| 2 | FixedPickTime | int | 否 | 固定提货时间, 单位: 秒, 为空时为0 | |
| 3 | FixedDropTime | int | 否 | 固定卸货时间, 单位: 秒, 为空时为0 | |

**外键依赖**:
| 本表字段 | 引用表 | 引用字段 |
|---|---|---|
| LocationCode | Location | Code |

**被依赖**: 无

---

### 11. WorkEffect（站点作业效率）

- **CSV 文件**: `WorkEffect.csv`
- **主键**: (`LocationCode`, `DimensionCode`)

| # | 字段名 | 数据类型 | 必填 | 说明 | 可选值 / 取值范围 |
|---|---|---|---|---|---|
| 1 | LocationCode | string | 是 | 站点编码 | 必须存在于 Location.Code |
| 2 | DimensionCode | string | 是 | 维度编码 | 必须存在于 Dimension.Code |
| 3 | PerHourProcessQuantity | double | 是 | 每小时处理量, 单位: 单位/小时, 最大支持小数点后4位 | |

**外键依赖**:
| 本表字段 | 引用表 | 引用字段 |
|---|---|---|
| LocationCode | Location | Code |
| DimensionCode | Dimension | Code |

**被依赖**: 无

---

## 车型

### 12. VehicleModel（车型）

- **CSV 文件**: `VehicleModel.csv`
- **主键**: `Code`

| # | 字段名 | 数据类型 | 必填 | 说明 | 可选值 / 取值范围 |
|---|---|---|---|---|---|
| 1 | Code | string | 是 | 车型编码，唯一标识 | |
| 2 | Name | string | 否 | 车型名称 | |
| 3 | DistMatrixCode | string | 是 | 使用的距离矩阵编码 | 必须存在于 DistMatrixCode.Code |

**外键依赖**:
| 本表字段 | 引用表 | 引用字段 |
|---|---|---|
| DistMatrixCode | DistMatrixCode | Code |

**被依赖**:
| 依赖方表 | 依赖方字段 | 关系 |
|---|---|---|
| VehicleModelDimensionValue | VehicleModelCode | 1 : N |
| VehicleModelLabelValue | VehicleModelCode | 1 : N |
| Vehicle | VehicleModelCode | 1 : N |
| LocationAvailableVehicle | VehicleModelCode | 1 : N |

---

### 13. VehicleModelDimensionValue（车型维度值）

- **CSV 文件**: `VehicleModelDimensionValue.csv`
- **主键**: (`VehicleModelCode`, `DimensionCode`)

| # | 字段名 | 数据类型 | 必填 | 说明 | 可选值 / 取值范围 |
|---|---|---|---|---|---|
| 1 | VehicleModelCode | string | 是 | 车型编码 | 必须存在于 VehicleModel.Code |
| 2 | DimensionCode | string | 是 | 维度编码 | 必须存在于 Dimension.Code |
| 3 | DimensionValue | double | 是 | 维度值 | |

**外键依赖**:
| 本表字段 | 引用表 | 引用字段 |
|---|---|---|
| VehicleModelCode | VehicleModel | Code |
| DimensionCode | Dimension | Code |

**被依赖**: 无

---

### 14. VehicleModelLabelValue（车型标签值）

- **CSV 文件**: `VehicleModelLabelValue.csv`
- **主键**: (`VehicleModelCode`, `LabelCode`)

| # | 字段名 | 数据类型 | 必填 | 说明 | 可选值 / 取值范围 |
|---|---|---|---|---|---|
| 1 | VehicleModelCode | string | 是 | 车型编码 | 必须存在于 VehicleModel.Code |
| 2 | LabelCode | string | 是 | 标签编码 | 必须存在于 Label.Code, 并且存在LabelApply.LabelCode=LabelCode & LabelApply.ApplyItem=`VehicleModel` |
| 3 | LabelValue | string | 是 | 标签取值 | 必须存在于 LabelValue(LabelCode, LabelValue) |

**外键依赖**:
| 本表字段 | 引用表 | 引用字段 |
|---|---|---|
| VehicleModelCode | VehicleModel | Code |
| (LabelCode, LabelValue) | LabelValue | (LabelCode, LabelValue) |
| LabelCode | LabelApply | (LabelCode, ApplyItem=`VehicleMode`l) |

**被依赖**: 无

---

## 承运商

### 15. Carrier（承运商）

- **CSV 文件**: `Carrier.csv`
- **主键**: `Code`

| # | 字段名 | 数据类型 | 必填 | 说明 | 可选值 / 取值范围 |
|---|---|---|---|---|---|
| 1 | Code | string | 是 | 承运商编码，唯一标识 | |
| 2 | Name | string | 否 | 承运商名称 | |

**外键依赖**: 无

**被依赖**:
| 依赖方表 | 依赖方字段 | 关系 |
|---|---|---|
| CarrierLabelValue | CarrierCode | 1 : N |
| Vehicle | CarrierCode | 1 : N |
| LocationAvailableVehicle | CarrierCode | 1 : N |

---

### 16. CarrierLabelValue（承运商标签值）

- **CSV 文件**: `CarrierLabelValue.csv`
- **主键**: (`CarrierCode`, `LabelCode`)

| # | 字段名 | 数据类型 | 必填 | 说明 | 可选值 / 取值范围 |
|---|---|---|---|---|---|
| 1 | CarrierCode | string | 是 | 承运商编码 | 必须存在于 Carrier.Code |
| 2 | LabelCode | string | 是 | 标签编码 | 必须存在于 Label.Code, 并且存在LabelApply.LabelCode=LabelCode & LabelApply.ApplyItem=`Carrier` |
| 3 | LabelValue | string | 是 | 标签取值 | 必须存在于 LabelValue(LabelCode, LabelValue) |

**外键依赖**:
| 本表字段 | 引用表 | 引用字段 |
|---|---|---|
| CarrierCode | Carrier | Code |
| (LabelCode, LabelValue) | LabelValue | (LabelCode, LabelValue) |
| LabelCode | LabelApply | (LabelCode, ApplyItem=`Carrier`l) |

**被依赖**: 无

---

### 17. Vehicle（车辆）

- **CSV 文件**: `Vehicle.csv`
- **主键**: (`CarrierCode`, `VehicleModelCode`)

| # | 字段名 | 数据类型 | 必填 | 说明 | 可选值 / 取值范围 |
|---|---|---|---|---|---|
| 1 | CarrierCode | string | 是 | 承运商编码 | 必须存在于 Carrier.Code |
| 2 | VehicleModelCode | string | 是 | 车型编码 | 必须存在于 VehicleModel.Code |
| 3 | Count | int | 是 | 车辆数量 | 为0时代表不可用,小于0代表不限制 |
| 4 | OriginLocationCode | string | 否 | 起始站点编码 | 不为空时, 必须存在于 Location.Code |
| 5 | DestinationLocationCode | string | 否 | 目标站点编码 | 不为空时, 必须存在于 Location.Code |

**外键依赖**:
| 本表字段 | 引用表 | 引用字段 |
|---|---|---|
| CarrierCode | Carrier | Code |
| VehicleModelCode | VehicleModel | Code |
| OriginLocationCode | Location | Code |
| DestinationLocationCode | Location | Code |

**被依赖**:
| 依赖方表 | 依赖方字段 | 关系 |
|---|---|---|
| LocationAvailableVehicle | (CarrierCode, VehicleModelCode) | 1 : N |

---

## 路由网络

### 18. DistMatrixCode（距离矩阵编码）

- **CSV 文件**: `DistMatrixCode.csv`
- **主键**: (`Code`)

| # | 字段名 | 数据类型 | 必填 | 说明 | 可选值 / 取值范围 |
|---|---|---|---|---|---|
| 1 | Code | string | 是 | 距离矩阵编码 | |
| 2 | Name | string | 否 | 距离矩阵编码名称 | |

**外键依赖**: 无

**被依赖**:
| 依赖方表 | 依赖方字段 | 关系 |
|---|---|---|
| DistMatrix | (DistMatrixCode) | 1 : N |
| VehicleModel | (DistMatrixCode) | 1 : N |


### 19. DistMatrix（距离矩阵）

- **CSV 文件**: `DistMatrix.csv`
- **主键**: (`FromLocationCode`, `ToLocationCode`, `DistMatrixCode`)

| # | 字段名 | 数据类型 | 必填 | 说明 | 可选值 / 取值范围 |
|---|---|---|---|---|---|
| 1 | FromLocationCode | string | 是 | 起始站点编码 | 必须存在于 Location.Code |
| 2 | ToLocationCode | string | 是 | 目标站点编码 | 必须存在于 Location.Code |
| 3 | DistMatrixCode | int | 是 | 距离矩阵编码（分组键） | 与 VehicleModel.DistMatrixCode 匹配 |
| 4 | Distance | int | 是 | 距离，单位：米 | >=0 & <=60000000 |
| 5 | Time | int | 是 | 耗时，单位：秒 | >=0 & <=2160000000  |

**外键依赖**:
| 本表字段 | 引用表 | 引用字段 |
|---|---|---|
| FromLocationCode | Location | Code |
| ToLocationCode | Location | Code |
| DistMatrixCode | DistMatrixCode | Code |

**被依赖**: 无

---

## 订单

### 20. CargoOrder（订单）

- **CSV 文件**: `CargoOrder.csv`
- **主键**: `Code`

| # | 字段名 | 数据类型 | 必填 | 说明 | 可选值 / 取值范围 |
|---|---|---|---|---|---|
| 1 | Code | string | 是 | 订单编码，唯一标识 | |
| 2 | Name | string | 否 | 订单名称 | |
| 3 | PickLocationCode | string | 是 | 提货站点编码 | 必须存在于 Location.Code |
| 4 | DropLocationCode | string | 是 | 卸货站点编码 | 必须存在于 Location.Code |
| 5 | EarliestPickDateTime | string | 是 | 最早提货时间, 时间格式%Y-%m-%d %H:%M | 必须 ≤ LatestPickDateTime |
| 6 | LatestPickDateTime | string | 是 | 最晚提货时间, 时间格式%Y-%m-%d %H:%M | 必须 ≥ EarliestPickDateTime |
| 7 | EarliestDropDateTime | string | 是 | 最早卸货时间, 时间格式%Y-%m-%d %H:%M | 必须 ≤ LatestDropDateTime |
| 8 | LatestDropDateTime | string | 是 | 最晚卸货时间, 时间格式%Y-%m-%d %H:%M | 必须 ≥ EarliestDropDateTime |

**外键依赖**:
| 本表字段 | 引用表 | 引用字段 |
|---|---|---|
| PickLocationCode | Location | Code |
| DropLocationCode | Location | Code |

**被依赖**:
| 依赖方表 | 依赖方字段 | 关系 |
|---|---|---|
| CargoSubOrder | CargoOrderCode | 1 : N |
| CargoSubOrderDimensionValue | CargoOrderCode | 1 : N |
| CargoSubOrderLabelValue | CargoOrderCode | 1 : N |

---

### 21. CargoSubOrder（子订单）

- **CSV 文件**: `CargoSubOrder.csv`
- **主键**: (`CargoOrderCode`, `CargoSubOrderCode`)

| # | 字段名 | 数据类型 | 必填 | 说明 | 可选值 / 取值范围 |
|---|---|---|---|---|---|
| 1 | CargoOrderCode | string | 是 | 父订单编码 | 必须存在于 CargoOrder.Code |
| 2 | CargoSubOrderCode | string | 是 | 子订单编码 | 同一父订单下唯一 |
| 3 | CargoSubOrderName | string | 否 | 子订单名称 | |
| 4 | Quantity | int | 是 | 货量 | |

**外键依赖**:
| 本表字段 | 引用表 | 引用字段 |
|---|---|---|
| CargoOrderCode | CargoOrder | Code |

**被依赖**:
| 依赖方表 | 依赖方字段 | 关系 |
|---|---|---|
| CargoSubOrderDimensionValue | (CargoOrderCode, CargoSubOrderCode) | 1 : N |
| CargoSubOrderLabelValue | (CargoOrderCode, CargoSubOrderCode) | 1 : N |

---

### 22. CargoSubOrderDimensionValue（子订单维度值）

- **CSV 文件**: `CargoSubOrderDimensionValue.csv`
- **主键**: (`CargoOrderCode`, `CargoSubOrderCode`, `DimensionCode`)

| # | 字段名 | 数据类型 | 必填 | 说明 | 可选值 / 取值范围 |
|---|---|---|---|---|---|
| 1 | CargoOrderCode | string | 是 | 父订单编码 | 与 CargoSubOrderCode 联合关联 CargoSubOrder |
| 2 | CargoSubOrderCode | string | 是 | 子订单编码 | 与 CargoOrderCode 联合关联 CargoSubOrder |
| 3 | DimensionCode | string | 是 | 维度编码 | 必须存在于 Dimension.Code |
| 4 | DimensionValue | double | 是 | 维度值 | |

**外键依赖**:
| 本表字段 | 引用表 | 引用字段 |
|---|---|---|
| (CargoOrderCode, CargoSubOrderCode) | CargoSubOrder | (CargoOrderCode, CargoSubOrderCode) |
| DimensionCode | Dimension | Code |

**被依赖**: 无

---

### 23. CargoSubOrderLabelValue（子订单标签值）

- **CSV 文件**: `CargoSubOrderLabelValue.csv`
- **主键**: (`CargoOrderCode`, `CargoSubOrderCode`, `LabelCode`)

| # | 字段名 | 数据类型 | 必填 | 说明 | 可选值 / 取值范围 |
|---|---|---|---|---|---|
| 1 | CargoOrderCode | string | 是 | 父订单编码 | 与 CargoSubOrderCode 联合关联 CargoSubOrder |
| 2 | CargoSubOrderCode | string | 是 | 子订单编码 | 与 CargoOrderCode 联合关联 CargoSubOrder |
| 3 | LabelCode | string | 是 | 标签编码 | 必须存在于 Label.Code, 并且存在LabelApply.LabelCode=LabelCode & LabelApply.ApplyItem=`CargoOrder` |
| 4 | LabelValue | string | 是 | 标签取值 | 必须存在于 LabelValue(LabelCode, LabelValue) |

**外键依赖**:
| 本表字段 | 引用表 | 引用字段 |
|---|---|---|
| (CargoOrderCode, CargoSubOrderCode) | CargoSubOrder | (CargoOrderCode, CargoSubOrderCode) |
| (LabelCode, LabelValue) | LabelValue | (LabelCode, LabelValue) |
| LabelCode | LabelApply | (LabelCode, ApplyItem=`CargoOrder`l) |

**被依赖**: 无

---


