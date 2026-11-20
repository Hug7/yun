# yun

车辆路径规划（VRP）求解器。输入基础数据（Scenario），构建规划模型（Load / Node / Activity），在约束下求解最优车次分配方案。

## 设计思想

场景层`scenario`记录“不可逆的客观事实”，问题层`problem`记录“可协商的规则与偏好”。问题层只定义规则与偏好，并据此对一个车次求值；车次的构造、插入与货单预检属于求解操作，不属于问题定义。

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

### 问题层

**Problem**（问题）:
场景与参数下的一套约束与偏好：给定一个车次，能回答它是否可行、代价多少。
_Avoid_: 求解器、引擎、上下文

**LoadEditor**（车次编辑）:
对车次的模型级操作：把订单装配成车次、给车次改派车型、把订单片段插入车次；只做评估，不占用车辆资源。
_Avoid_: 算子（ruin / repair 才是搜索算子）、工厂、生成器

### 模型层

**Load**（车次）:
一次车辆出车计划。包含一条 Node 链（行驶路径）、一辆分配车辆、路由级聚合属性（LoadRouteProfile）和约束评分。构造时即建立双哨兵 depot：起始车场 Node 和目的车场 Node。`VisualPlanSummary` 中「总车次数 = loads.size()」。
_Avoid_: 路线、任务、运次、车辆（Vehicle，车次的承载工具）

**Node**（停靠点）:
Load 的 Node 链上的一个停留点。含位置、计划时间窗（ptws）、从上一 Node 到此的 travel_dist/travel_time，以及一个 Activity 链表。首尾两个 Node 为 depot 哨兵。
_Avoid_: 站点、访问点

**Activity**（活动）:
在某个 Node 上针对特定 Order 的动作：取货（pick）或送货（drop）。每个 Order 产生一对 Activity，pair 创建函数保证取货在前、送货在后。

**Depot**（车场）:
Load 的 Node 链的起点或终点。由 Vehicle 的 origin_loc / dest_loc 决定；车辆未指定车场时使用预留空站点，其到任意位置的距离和时间均为 0。

**Order**（订单）:
模型层的取送货任务，是对数据层 CargoOrder 的求解表示。含取/送 Location、时间窗、维度、标签。一个 CargoOrder 可能被拆分成多个 Order 分别分配给不同 Load。

**Segment**（订单片段）:
按指定访问顺序排列的一组 Order，是插入操作的单位。
_Avoid_: 批次、分组

**InsertSlot**（插入槽位）:
订单片段在车次停靠点链上的一个候选插入位置，由所属区（取货区/卸货区）和序号确定。

**汇总口径约定**：凡「订单数 / 总订单数 / 各维度订单数」一律指**货单数**（按 `Order::cargo_orders` 展开计数），不是 `Order` 父单元数。

**Constraint Score**（约束评分）:
三重约束体系——硬约束（HardConstrScore：不可违反，违规则不可行）、软约束（SoftConstrScore：可违反但有惩罚）、成本约束（CostConstrScore：存在计费项时必须有费用且计入目标函数）。每个 Score 携带编码（code）、权重（weight）和值（value），聚合在 Load 的 LoadAttrConstraint 中。

### 求解上下文

**Request**（请求）:
一次求解的运行实例，由 `request_id`（`%Y%m%d%H%M%S_{进程标识}_{序号}`）标识。本次请求产生的全部产物（可视化文件、日志）都落在以它命名的输出目录里。
_Avoid_: 运行、会话、任务

### 求解过程

**Workspace**（工作空间）:
一次求解过程中可变的工作状态：当前的 Load 集合与尚未指派的 Order 集合，可压栈暂存以支持回退。求解期间被反复改写，本身不是产物。
_Avoid_: 上下文、缓存

**Initial Solution**（初始解）:
用构造启发式（如 K 近邻）将未分配的订单插入到提供的 Load 或者构造新的 Load。
_Avoid_: 构造方案、初始方案、构造解

**Solution**（方案）:
一次求解的最终产物：Load 集合、车辆资源占用、未指派的 Order，以及由三重 Constraint Score 汇总出的总成本。
_Avoid_: 结果、答案

**Local Search**（邻域搜索）:
在已指派订单集合内做局部重排的改进过程：relocate/or-opt（搬 1 个或一段连续可移动单元，默认 1 / 2~3）、exchange（交换两个单元）、2-opt（反转同车一段连续单元）。relocate/or-opt 与 exchange 再按作用域拆成路线内部（intra-route）与跨路线（inter-route）两个独立算子（见 LsRouteScope），默认共 7 个算子实例。只改已指派订单的顺序或所属车次，不插入未指派订单、不改车型、不增删车次（v1 禁止搬空车次）。当前为全量搜索：每个 pass 枚举全部候选、只提交最优改进 move，直到一整轮无改进，不设时间/次数/候选上限；评估前有两层剪枝：订单 KNN 图（move 新建的相邻停车点对至少一对是 K 近邻边才保留，近邻数取 `LsParameter::knn_neighbor_count`，图由 `LsSearch` 持有、首次搜索时构建一次后复用）与里程增量（move 生效后受影响车次的里程增加就丢弃，阈值为 0）。改进策略由 `LsImprovement` 决定：`Best`（默认）扫完一轮取最优提交，`First` 遇到第一个改进候选就提交；每个 pass 的起点算子按 pass 序号轮转。评估次数可用 `LsParameter::max_evaluated` 封顶（`0` 表示不限），计量口径是所有 report 的汇总 `evaluated` 数（ALNS 内嵌多次调用共享），达到上限时提交本 pass 已找到的最优 move 并以 `max_evaluated` 终止。
_Avoid_: 与 ALNS 的 ruin/repair 混用（ruin 破坏再 repair 重建，local search 只做局部重排）

**LsMove**（邻域 move）:
描述一次局部改动的纯数据：源车次/起始单元/单元数，以及目标车次与槽位（relocate）、交换对（exchange）或反转块（2-opt）。算子只产 LsMove，由引擎评估并提交。
_Avoid_: 与 ALNS 的 ruin/repair 算子（它们直接改 Solution）混用

**LsNeighborhood**（邻域适配器）:
pattern 中立的结构视图与编辑端口：可移动单元数、全部可行目标车次/槽位、车次拷贝、apply 与 canonicalize。隔离 SPMD 与多模式的装载结构差异；v1 只有 SPMD 实现（可移动单元 = drop 节点）。
_Avoid_: 车次编辑（LoadEditor 负责装配订单/改派车型，适配器只做结构编辑）

**LsRouteScope**（邻域作用范围）:
邻域算子的作用域：`Intra` 只在同一车次内搬动/交换单元，`Inter` 只在不同车次之间搬动/交换。
由算子 code（`LsIntraRoute*` / `LsInterRoute*`）决定，不暴露成 DSL 参数。  
_Avoid_: 邻域作用的车次范围。

### 策略编排

**策略方法 (Strategy Method)**：
Lua DSL 注册的顶层 C++ 方法（如 `construct_knn` / `alns` / `local_search` / `snapshot`），是求解编排的顶层步骤。
_Avoid_：ALNS 内部的 ruin/repair 算子、local search 内部的邻域算子（都是更细的粒度）。

**策略方法耗时**：
不在 `loads`/`unassigned_orders` 中推导，须由 solver 在各策略方法调用前后埋点（chrono）累计，落入 `VisualPlanSummary::strategy_timings` 与 `total_running_time_second`。