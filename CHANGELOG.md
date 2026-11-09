# 更新日志

记录对外可见的变更；版本号格式与发布流程见
[docs/versioning.md](docs/versioning.md)。

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
