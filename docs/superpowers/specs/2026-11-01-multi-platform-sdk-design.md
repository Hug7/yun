# 多平台预编译 SDK 设计

> 状态：已确认（2026-11-01）
> 范围：`yun` 仓库；`yunpyopt` 的消费改造与端到端验证不在本轮。

## 1. 目标

把当前只支持 macOS arm64 的预编译 SDK，扩展为 macOS arm64、Linux x86_64、Linux arm64、Windows x86_64 四个平台的产物，并给出规范的 CMake 消费契约。

约束（已确认）：

- 平台矩阵只做 4 个正式产物；Windows arm64、macOS Intel 不做。
- 只改 `yun` 仓库；不改 `yunpyopt`。
- 不新增 SDK CI，不在 yun 侧做发布前消费自检；端到端验证由 `yunpyopt` 负责。
- Windows 消费方只支持 clang-cl（与 yun 的编译器政策一致），不承诺 MSVC。
- SDK 采用静态库 + CMake package，不做共享库。

## 2. 平台矩阵

| 平台 | 构建环境 | 平台标签 | 归档器 | 库文件 |
|---|---|---|---|---|
| macOS arm64 | `macos-15`，AppleClang，deployment 14.0 | `macos-arm64` | `libtool -static` | `libyun_sdk.a` |
| Linux x86_64 | `manylinux_2_28_x86_64` + `gcc-toolset-14` | `linux-x86_64` | GNU `ar -M` | `libyun_sdk.a` |
| Linux arm64 | `manylinux_2_28_aarch64` + `gcc-toolset-14` | `linux-arm64` | GNU `ar -M` | `libyun_sdk.a` |
| Windows x86_64 | `windows-2022`，clang-cl + MSVC STL（/MD） | `windows-x86_64` | `llvm-lib` | `yun_sdk.lib` |

产物命名统一为 `yun-sdk-<version>-<tag>.tar.gz`；Release 附带合并后的 `SHA256SUMS`。tarball 保持扁平布局（`include/`、`lib/`、`src/`、`CMakeLists.txt`、`VERSION`、`SDK_PLATFORM`、`LICENSE`）。

## 3. SDK 内容与消费契约

```text
include/                    yun 五个组件头文件（include/yun/<component>/）
                            + vendored fmt/spdlog/yaml-cpp/magic_enum/sol2/libfork/lua 头文件
lib/libyun_sdk.a | yun_sdk.lib
lib/cmake/yun/yunConfig.cmake
lib/cmake/yun/yunConfigVersion.cmake
src/yun_solver_binding.cpp
CMakeLists.txt              SDK 根项目：构建 yunpyopt._core
VERSION
SDK_PLATFORM
LICENSE
```

`yunConfig.cmake` 导出 `yun::sdk` 导入目标：

- include 目录：`include` + 五个组件目录；
- `cxx_std_20`；
- 编译定义：`SPDLOG_COMPILED_LIB`、`SPDLOG_FMT_EXTERNAL`、`YAML_CPP_STATIC_DEFINE`；
- 系统库：Linux `Threads::Threads;${CMAKE_DL_LIBS};m`；macOS `Threads::Threads`；Windows 无；
- 版本文件使用 same-minor 兼容策略（0.x 下 minor 是 ABI 断点）。

消费方式：

```cmake
find_package(yun CONFIG REQUIRED)   # CMAKE_PREFIX_PATH 或 HINTS 指向 SDK 根
target_link_libraries(your_target PRIVATE yun::sdk)
```

SDK 根 `CMakeLists.txt` 使用 `find_package(yun CONFIG REQUIRED HINTS ...)`，再用 pybind11 构建 `_core`。

## 4. 打包实现

### 4.1 清单由 CMake 生成

不再解析 `link.txt`（Ninja/Windows 下不存在），也不依赖 `compile_commands.json`。

根 `CMakeLists.txt` 增加 `YUN_BUILD_SDK`，引入 `cmake/sdk.cmake`：

- 自己的静态库：`yun_solver`、`yun_algorithm`、`yun_problem`、`yun_scenario`，路径用 `$<TARGET_FILE:...>`；
- Conan 静态库：从 `yun_solver` 的链接闭包递归遍历，展开 `$<LINK_ONLY:...>` 与 `$<$<CONFIG:Release>:...>`，读取 `IMPORTED_LOCATION_RELEASE`；
- 头文件目录：从 `fmt::fmt`、`spdlog::spdlog`、`yaml-cpp::yaml-cpp`、`magic_enum::magic_enum`、`sol2::sol2`、`libfork::libfork`、`lua::lua` 的 `INTERFACE_INCLUDE_DIRECTORIES` 求值；
- 用 `file(GENERATE)` 写出 `sdk-archives-<config>.txt`、`sdk-includes-<config>.txt`。

### 4.2 build_sdk.py

- 平台/架构检测 → 平台标签；
- 统一通过 `cmake/conan.cmake` 配置，删除 `CONAN_HOST_PROFILE`，让 provider 正确探测 Windows clang-cl 的 runtime/编译器；
- 配置项：`CMAKE_BUILD_TYPE=Release`、`YUN_BUILD_SDK=ON`、`YUN_BUILD_TESTS=OFF`、`YUN_BUILD_PYTHON=OFF`、`CMAKE_SKIP_RPATH=ON`；
- Linux：`CMAKE_POSITION_INDEPENDENT_CODE=ON`；Windows：Ninja + clang-cl；macOS：arm64 + deployment 14.0；
- 按平台调用 `libtool`、`ar -M`、`llvm-lib` 合并归档；
- 按清单复制头文件、生成 CMake package、写元数据、打 tar.gz；
- 构建 SDK 本身不再需要 Python 解释器，删除 `--python` 参数。

## 5. 发布流水线

`sdk-release.yml`：

1. `version`：从 tag 或 `workflow_dispatch` 输入解析版本，校验与 `CMakeLists.txt` 一致；
2. `build-macos`：`macos-15`；
3. `build-linux`：矩阵 x86_64/arm64，manylinux 容器，`gcc-toolset-14`；
4. `build-windows`：`windows-2022` + `ilammy/msvc-dev-cmd` + LLVM + Ninja + Python 3.11；
5. `release`：下载全部 tarball，生成合并 `SHA256SUMS`，tag 触发时发布；`workflow_dispatch` 只保留 artifact。

## 6. 版本与文档

- `project(yun VERSION ...)` 与 `conanfile.py` 升到 `0.2.0`；
- 新增 `docs/sdk_guide.md`；
- 更新 `README.md` §5、`docs/platform_support.md` 已知限制。

## 7. 风险

| 风险 | 影响 | 缓解 |
|---|---|---|
| CMake 目标图遍历依赖 Conan genex 形式 | 清单为空/漏库 | 已用 mock 验证；失败时回退到解析 `link.txt`（仅 Makefile） |
| manylinux 容器内 GitHub Actions 兼容性 | Linux job 启动失败 | 失败时退回 `docker run` 方案 |
| Windows `llvm-lib` 合并行为 | 归档失败 | 优先 `llvm-lib`，回退 `lib.exe` |
| Linux 静态库非 PIC、缺 `dl` | 消费方链接失败 | 构建开 PIC；package 加 `dl`/`m` |
| 不做发布前自检 | 打包错误晚发现 | 由 yunpyopt 端到端验证；yun 侧本地验证 macOS/manylinux |
