# 更新日志

记录对外可见的变更；版本号格式与发布流程见
[docs/versioning.md](docs/versioning.md)。

## [Unreleased]

### Added

- 新增硬约束预检开关 `hard_constr_precheck`（默认关闭）：插入订单片段前，先用与插入位置无关的
  硬约束（`VehicleCapacity`、`AvailableVehicle`、`MaxPickNodeCount`、`MaxDropNodeCount`、
  `LabelValueExclusive`、`LabelValueCompatible`、`LabelValueMaxCount`）做保守判定，能确定
  "插入后必然违背"时直接剪掉该（车次, 订单片段）组合，省去枚举插入位置与逐槽求值。预检只剪
  确定违背的组合、无法判定时回退完整求值，因此不改变可行性判定结果与目标值。 (#32)

- 新增邻域搜索（local search）：独立策略步 `local_search(cfg)`，支持 relocate / or-opt /
  exchange 三类邻域，且每类都按作用域拆成路线内部（intra-route）与跨路线（inter-route）两个
  独立算子（`LsIntraRouteRelocate`、`LsInterRouteRelocate`、`LsIntraRouteExchange`、
  `LsInterRouteExchange`），另有同车 2-opt（`LsTwoOpt`），默认 7 个算子实例；v1 仅支持
  SPMD 装卸模式。当前为全量搜索：每个 pass 枚举并评估全部候选、只提交本 pass 最优的改进
  move，不设时间预算、move 数上限或候选上限，直到一整轮没有改进为止。候选在评估前有两层
  剪枝：订单 K 近邻图剪枝（新建相邻停车点对至少一对是 K 近邻边才保留）与里程增量剪枝
  （move 生效后受影响车次里程增加即丢弃，阈值为 0），日志新增 `pruned`、`dist_pruned` 计数。
  支持 `improvement = "best" | "first"` 选择改进策略（默认 `best`：扫完一轮取最优；`first`：
  遇到第一个改进候选立即提交），每个 pass 的起点算子按 pass 序号轮转，日志新增
  `improvement` 字段；新增 `max_evaluated` 评估次数上限（`0` 表示不限，按所有 report 的
  汇总 `evaluated` 数计量，达到上限时提交本 pass 已找到的最优 move 后以 `max_evaluated`
  终止）。KNN 剪枝的近邻数可用 `knn_neighbor_count` 配置（默认 10，`0` 表示不做 KNN 剪枝），图由 `LsSearch` 持有：
  首次搜索时构建一次，ALNS 内嵌搜索整个求解复用，不再每次局部搜索重建。 (#10)
- `alns` 支持可选内嵌 local search：`alns { local_search = { ... } }`。 (#10)
- 新增两个常规硬约束 `LabelValueExclusive`（标签值互斥）与 `LabelValueCompatible`（标签值可拼），
  支持 `Order` 标签与 `Location` 标签；Location 可用 `side` 选择 `Pick` / `Drop` / `Both`。
  互斥按组内两两不可同车判定，不做跨组传递合并；可拼按“不同生效组不可同车”判定，
  未列出的取值自动归入默认组。约束在构造时预计算每个取值的不可拼位图，判定时与车次
  标签位图求交；新增不可行原因 1011–1016。参数说明见
  [docs/parameter_reference.md](docs/parameter_reference.md)。(#47)
- 新增常规硬约束 `LabelValueMaxCount`（标签值种类数上限），支持 `Order` 与 `Location` 标签；
  Location 可用 `side` 选择 `Pick` / `Drop` / `Both`，`Both`（默认）按提货侧与卸货侧合并去重统计。
  新增不可行原因 1017–1020；同时移除未使用的 1007，原 1008–1017 前移一位为 1007–1016，保持编码连续。
  参数说明见 [docs/parameter_reference.md](docs/parameter_reference.md)。 (#48)

### Changed

- 不可解原因码重排：移除未使用的 1007 `DISTINCT_LABEL_VALUE_MAX_COUNT`，原 1008–1017 前移一位；
  对外输出的 `ReasonCode` 数值随之变化。 (#48)
- alns repair 公用插入方法改为 delta insert，用于缓解全量的无效插入动作。 (#31)
- Problem 职责简化，将 insert 和 check order 功能移出。 (#33)
- alns 参数改由策略层转换为 POD，算法层脱离 Lua。 (#28)
- 构造方法 knn 从 solver 层移动到 algorithm 层。 (#27)
- imeWindow和TimeWindowPlan从“堆对象 + 裸指针”改成值类型。 (#39)
- `Bitset` 从“堆对象 + `unique_ptr`”改为带小对象优化（SBO）的值类型：位宽 ≤64 时零堆分配，
  超过 64 时退化为单块动态数组，无硬上限；移除 `Bitset::UPtr` 及指针接口，`BitsetUtils`
  改为按值返回。`AvailableVehicle`、`Order::available_vehicle_bitset`、`LoadRouteProfile`
  的位图字段同步改为值语义；`LabelsetValueBitset` 改为按 labelset 共享的扁平 arena，每个
  标签占一段连续 word，位图分配从“每标签两个堆块”降为“每个 labelset 一块”。
  `Order::labelset_value_bitset` 保留 `unique_ptr`，用于表达“无标签位图”。相关公开头文件接口
  有变更，消费方需重新构建。 (#43)
- 优化 DistMatrix 的尺寸与边界。 (#45)

### Fixed

- alns 最大停滞迭代次数没有生效。 (#34)
- 统一时间窗交集的处理逻辑。(#40)
- 时间窗冻结表内存无上限增长优化。(#38)
- 修正Load::change_vehicle 的缓存失效不完整。(#44)

## [0.3.1] - 2026-11-09

### Changed

- 新增版本信息的全局管理，解决项目中多文件维护版本信息容易出现的不一致问题 (#29)。

### Fixed

- 修复 knn 构造解不满足幂等性设计的问题 (#25)。

## [0.3.0] - 2026-11-08

### Added

- 新增 ruin 算子 `RuinWorstRemovalOrder` (#16)。
- alns 不设置算子时使用默认算子，并打印参数 (#15)。
- alns 算子权重改用 Ropke-Pisinger σ 分数并改为分段 EWMA (#19)。

### Changed

- ruin 与 repair 算子的笛卡尔积解耦 (#14)。
- **ABI 不兼容**：消费方需重新构建，并把 `find_package(yun ...)` 的请求版本改到 `0.3`。

### Fixed

- 修复 SA 概率接受逻辑漏洞，并引入最大迭代停滞代数 `max_stagnate_iter` 重置当前解 (#17)。
- 修复默认 `work_count` 分片数计算 bug (#18)。
- 修复 knn 车辆资源不足时未指派的订单丢失 (#9)。
- 修正与实际不符的漂移文档 (#12)。

## [0.2.0] - 2026-11-02

### Added

- 多平台预编译 SDK：macOS arm64、Linux x86_64/arm64、Windows x86_64。
- Python 扩展打包流程。

### Changed

- `build_sdk.py` 改为平台通用，SDK 产物由 release workflow 构建。
- 兼容 Windows clang-cl：移除 `__int128`、`strncasecmp` 等 GNU 专有用法，`long` 改为 `std::int64_t`。

### Fixed

- 构建脚本强制 UTF-8 stdio，避免 Windows 控制台编码错误。

## [0.1.0] - 2026-10-28

### Added

- 初始版本：场景、问题、算法与求解器的 C++ 实现，以及 Lua DSL 与计费框架。
