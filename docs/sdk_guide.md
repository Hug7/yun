# 预编译 SDK 使用手册

`yun` 的 GitHub Release 提供四个平台的预编译 SDK。SDK 是一个**单体静态库 + CMake package**，消费方不需要 yun 源码、Conan 或构建脚本，只需要自己的编译器与 CMake。

> SDK 的端到端消费验证（下载、构建 binding、打包 wheel）由 [`yunpyopt`](https://github.com/Hug7/yunpyopt) 负责；本文只定义产物与消费契约。

## 1. 平台矩阵

| 平台 | 产物 | 构建基线 | 库文件 |
|---|---|---|---|
| macOS arm64 | `yun-sdk-<version>-macos-arm64.tar.gz` | macOS 14.0+，AppleClang 16+ | `lib/libyun_sdk.a` |
| Linux x86_64 | `yun-sdk-<version>-linux-x86_64.tar.gz` | manylinux_2_28（glibc ≥ 2.28），GCC 14 | `lib/libyun_sdk.a` |
| Linux arm64 | `yun-sdk-<version>-linux-arm64.tar.gz` | 同上 | `lib/libyun_sdk.a` |
| Windows x86_64 | `yun-sdk-<version>-windows-x86_64.tar.gz` | clang-cl 17+ + MSVC STL（VS 2022 17.10+），动态 CRT（/MD） | `lib/yun_sdk.lib` |

每个平台的 tarball 都是扁平布局：

```text
include/
  yun/{common,scenario,problem,algorithm,solver}/
  fmt/ spdlog/ yaml-cpp/ magic_enum/ sol/ libfork/ lua.hpp ...
lib/
  libyun_sdk.a            # Windows: yun_sdk.lib
  cmake/yun/yunConfig.cmake
  cmake/yun/yunConfigVersion.cmake
src/yun_solver_binding.cpp
CMakeLists.txt
VERSION
SDK_PLATFORM
LICENSE
```

`libyun_sdk.a` 已经合并了 `yun_solver`、`yun_algorithm`、`yun_problem`、`yun_scenario` 以及 fmt/spdlog/yaml-cpp/lua 的静态库。`libfork`、`sol2`、`magic_enum` 是 header-only，只出现在 `include/`。

## 2. 消费方要求

| 平台 | 编译器 | 运行时 |
|---|---|---|
| macOS | AppleClang 16+ | macOS 14.0+ |
| Linux | GCC 13.2+ 编译 | glibc ≥ 2.28；SDK 以 GCC 14 构建，运行时 libstdc++ 需不低于 GCC 14（或在打包时静态链接/随包分发） |
| Windows | clang-cl 17+ | MSVC 动态运行时（/MD） |

约束：

- SDK 是**同一编译器家族**的静态库。Linux 用 GCC、macOS 用 AppleClang、Windows 用 clang-cl；不做跨编译器 ABI 承诺。
- Windows 消费方**只支持 clang-cl**，不支持 MSVC `cl.exe`（静态库含 clang 运行时符号，例如 `__int128` 除法）。
- 不承诺跨 SDK 版本的二进制兼容；升级 SDK 版本时消费方必须重新构建。

## 3. 在 CMake 中使用

解压 tarball 后，把 SDK 根目录加入 `CMAKE_PREFIX_PATH`（或在 `find_package` 里用 `HINTS`）：

```cmake
list(APPEND CMAKE_PREFIX_PATH "/path/to/yun-sdk-<version>-linux-x86_64")
find_package(yun CONFIG REQUIRED)   # 需要版本约束时请求 SDK 的 major.minor

add_executable(demo main.cpp)
target_link_libraries(demo PRIVATE yun::sdk)
```

`yun::sdk` 已经携带：

- 头文件目录（`include` + 五个组件目录）；
- `cxx_std_20`；
- 编译定义 `SPDLOG_COMPILED_LIB`、`SPDLOG_FMT_EXTERNAL`、`YAML_CPP_STATIC_DEFINE`；
- 系统库：Linux `Threads::Threads`、`dl`、`m`；macOS `Threads::Threads`；Windows 无。

C++ 侧入口是 `s_solver.h` 的 `Solver`：

```cpp
#include "s_solver.h"

int main() {
  Solver solver("/path/to/case", "/path/to/log", "warning");
  solver.solve();
  return 0;
}
```

### 3.1 构建 Python 扩展

SDK 自带 `CMakeLists.txt`，用于把 `src/yun_solver_binding.cpp` 构建为 `yunpyopt._core`。它需要消费方提供 Python 3.11 开发头文件与 pybind11：

```bash
cmake -S /path/to/yun-sdk \
  -B build/core \
  -DCMAKE_BUILD_TYPE=Release \
  -DPython3_EXECUTABLE=/path/to/python3.11 \
  -Dpybind11_DIR="$(python -c 'import pybind11; print(pybind11.get_cmake_dir())')"
cmake --build build/core --parallel
```

产物是 `build/core/_core*.so`（Windows 为 `_core*.pyd`）。

## 4. 本地构建 SDK

脚本必须在目标平台上执行；它用 CMake + Conan 编译 `yun_solver`，读取 CMake 生成的归档/头文件清单，然后合并静态库、生成 CMake package 并打包。版本取自仓库根目录 `VERSION`。

```bash
python3.11 scripts/build_sdk.py --out-dir dist/sdk --clean
```

产物：

```text
dist/sdk/yun-sdk-<version>-<platform>.tar.gz
dist/sdk/yun-sdk-<version>-<platform>.tar.gz.sha256
```

### 4.1 Linux（manylinux_2_28）

SDK 在 manylinux 容器中构建，以保证 glibc 2.28 基线：

```bash
docker run --rm \
  -v "$PWD:/src:ro" \
  -v "$PWD/build/conan-linux:/conan" \
  -v "$PWD/dist:/out" \
  -e CONAN_HOME=/conan \
  quay.io/pypa/manylinux_2_28_x86_64 \
  bash -lc '
    set -euo pipefail
    source /opt/rh/gcc-toolset-14/enable
    mkdir -p /tmp/src
    tar -C /src --exclude=build --exclude=.git --exclude=dist -cf - . | tar -C /tmp/src -xf -
    cd /tmp/src
    PY=/opt/python/cp311-cp311/bin/python
    "$PY" -m pip install -q "conan>=2.33,<3" "cmake>=3.24"
    export PATH="$(dirname "$PY"):$PATH"
    conan profile detect --force
    "$PY" scripts/build_sdk.py --out-dir /out --clean
  '
```

arm64 用 `quay.io/pypa/manylinux_2_28_aarch64`，并在 arm64 Linux 上运行。

### 4.2 Windows

在 VS 2022 开发者环境（`ilammy/msvc-dev-cmd` 或 “x64 Native Tools Command Prompt”）中，确保 `clang-cl`、`llvm-lib`、`ninja`、CMake、Conan 可用：

```bat
python scripts\build_sdk.py --out-dir dist --clean
```

## 5. 发布流程

`.github/workflows/sdk-release.yml` 的版本一律来自仓库根目录 `VERSION`，workflow 内不保存版本字面量：

1. `version`：读取 `VERSION`；tag 触发时要求 `tag == v${VERSION}`，否则立即失败；
2. `build-macos`、`build-linux`（x86_64/arm64 矩阵）、`build-windows`：构建并上传各平台 tarball；
3. `release`：汇总四个平台产物，生成合并的 `SHA256SUMS`，tag 触发时发布到 GitHub Release。

手动演练：

```text
Actions → SDK Release → Run workflow
```

只构建 artifact，不创建 Release。正式发布用 tag：

```bash
VERSION="$(cat VERSION)"
git tag -a "v${VERSION}" -m "yun ${VERSION}"
git push origin "v${VERSION}"
```

`release` job 使用 `release` environment（可配置 Required reviewers 做人工审批）。版本升级与 CHANGELOG 的完整流程见 [versioning.md](versioning.md)。

## 6. 版本与 ABI

- 版本唯一来源是根目录 `VERSION`；`conanfile.py`、workflow 与文档都从它读取，不得各自维护版本（`conanfile.py` 通过 `_project_version()` 直接读 `VERSION`）。
- `yunConfigVersion.cmake` 使用 same-minor 兼容策略：0.x 下 minor 是 ABI 断点，patch 版本视为兼容。
- 不承诺跨 SDK 版本的二进制兼容；升级 minor 时消费方必须重新构建，并把 `find_package(yun ...)` 的请求版本改到新的 `major.minor`。
- 不要覆盖已发布版本的 SDK 资产；C++ 变化时升版本重新发布。
- 消费方（如 `yunpyopt`）应固定 `yun-sdk-version.txt`，并用 `SHA256SUMS` 校验下载。
- 历史版本的变更记录见 [CHANGELOG.md](../CHANGELOG.md)。

## 7. 已知限制

- 不支持 Windows arm64 与 macOS Intel 的 SDK 产物（核心 CI 有 Windows arm64 job，但 SDK 暂不发布）。
- Windows 消费方不支持 MSVC。
- SDK 不包含 Python 运行时或 pybind11；它们由消费方提供。
- yun 仓库不提供 SDK 的消费冒烟测试；端到端验证由 `yunpyopt` 的 CI/发布流程负责。
