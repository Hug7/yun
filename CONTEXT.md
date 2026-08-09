# yun — VRP Solver

车辆路径规划（VRP）求解器。输入基础数据（Scenario），构建规划模型（Load / Node / Activity），在约束下求解最优车次分配方案。

## 语言

### 输入数据

**Scenario**（场景）:
一次求解任务的输入数据集：位置、距离矩阵、车型、承运商、订单等。加载和预处理完成后不可修改。
_Avoid_: 数据源、输入集

**Location**（位置）:
一个地理点位，含经纬度、工作计划、标签、可用车辆集合。
_Avoid_: 站点、坐标点、点

**CargoOrder**（货物订单）:
数据层的取送货任务。有一个提货 Location、一个卸货 Location、时间窗、子订单明细。求解时会被合并或拆分后转为模型层的 Order。
_Avoid_: 运单、工单

**Vehicle**（车辆）:
一个车辆实例，属于某个承运商、某个车型，有数量、起始车场（origin_loc）和目的车场（dest_loc）。车场未指定时使用预留空站点（距离/时间恒为 0）。

**VehicleModel**（车型）:
车型定义：维度属性（载重、体积等）、标签、关联的距离矩阵编码（dist_matrix_code）。同一 Node 序列在不同车型下可能产生不同的行驶距离和时间。

**DistMatrix**（距离矩阵）:
两个位置之间的行驶距离（m）和耗时（s）。按 DistMatrixCode 区分不同类型（如不同车型的路网）。未连接的两个位置返回 MAX 值。

**Dimension**（维度）:
带精度的数值属性（如重量 kg、体积 m³），在实体间以 `vector<long>` 形式承载，精度通过乘以 10^precision 转换为整数存储。
_Avoid_: 属性、字段

**Label**（标签）:
分实体类型（Location / VehicleModel / Order / Carrier）的枚举属性，每个 Label 下有一组 LabelValue。通过 LabelsetValueBitset 支持高效的集合并集运算和位图过滤。
_Avoid_: 标记、特征

### 模型层

**Load**（车次）:
一次车辆出车计划。包含一条 Node 链（行驶路径）、一辆分配车辆、路由级聚合属性（LoadRouteProfile）和约束评分。构造时即建立双哨兵 depot：起始车场 Node 和目的车场 Node。
_Avoid_: 路线、任务、运次

**Node**（停靠点）:
Load 的 Node 链上的一个停留点。含位置、计划时间窗（ptws）、从上一 Node 到此的 travel_dist/travel_time，以及一个 Activity 链表。首尾两个 Node 为 depot 哨兵。
_Avoid_: 站点、访问点

**Activity**（活动）:
在某个 Node 上针对特定 Order 的动作：取货（pick）或送货（drop）。每个 Order 产生一对 Activity，pair 创建函数保证取货在前、送货在后。

**Depot**（车场）:
Load 的 Node 链的起点或终点。由 Vehicle 的 origin_loc / dest_loc 决定；车辆未指定车场时使用预留空站点，其到任意位置的距离和时间均为 0。

**Order**（订单）:
模型层的取送货任务，是对数据层 CargoOrder 的求解表示。含取/送 Location、时间窗、维度、标签。一个 CargoOrder 可能被拆分成多个 Order 分别分配给不同 Load。

**Constraint Score**（约束评分）:
三重约束体系——硬约束（HardConstrScore：不可违反，违规则不可行）、软约束（SoftConstrScore：可违反但有惩罚）、成本约束（CostConstrScore：计入目标函数）。每个 Score 携带编码（code）、权重（weight）和值（value），聚合在 Load 的 LoadAttrConstraint 中。
