# 平台支持

## 支持矩阵

| 平台 | 架构 | 编译器 | 状态 | CI job |
|---|---|---|---|---|
| macOS | arm64 | AppleClang 16+（Xcode 16+） | 支持（CI 通过） | `Build & Test (macOS arm64 AppleClang)` |
| Linux | x86_64 | GCC 14 | 支持（CI 通过） | `Build & Test (Linux x86_64 GCC 14)` |
| Linux | arm64 | GCC 14 | 支持（CI 通过；另用 manylinux arm64 Docker 验证过） | `Build & Test (Linux arm64 GCC 14)` |
| Windows | x86_64 | clang-cl 17+（MSVC STL，VS 2022 17.10+） | 支持（CI 通过） | `Build & Test (Windows x64 clang-cl)` |
| Windows | arm64 | clang-cl 17+（MSVC STL，ARM64 SDK） | 实验性：CI 已通过，`windows-11-arm` 仍为 public preview | `Build & Test (Windows arm64 clang-cl)` |

**编译器政策：** Linux 仅支持 GCC；macOS 使用 AppleClang；Windows 仅支持 clang-cl（Clang + MSVC ABI/STL）。真正的 MSVC（`cl.exe`）和 MinGW GCC 都会在 CMake 配置阶段被拒绝；Windows 上 clang-cl 只使用 MSVC 的 STL、链接器和 Windows SDK，不使用 MSVC 编译器。Linux 上使用 Clang 也会被拒绝。

**CI 说明：** 5 个 `Build & Test` job 各自完成配置、构建、8 个 ctest 和 `C_SUC_001` 功能测试；3 个 ASan job 覆盖 macOS 与 Linux x86_64/arm64（`ASAN_OPTIONS=detect_leaks=0`），Windows 不做 ASan。

**最低版本：** C++20；CMake ≥ 3.24；Conan `>=2.33,<3`；Linux GCC ≥ 13.2；macOS AppleClang ≥ 16；Windows clang-cl ≥ 17。以上 Windows/AppleClang/GCC 下限同时是 `CMakeLists.txt` 配置阶段的硬门禁。

## 各平台构建

### macOS

```bash
./RUN.sh --mode debug
ctest --test-dir build/Debug --output-on-failure
```

### Linux（GCC 14）

```bash
sudo apt-get install -y g++-14 ninja-build
cmake -S . -B build/ci -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DYUN_BUILD_TESTS=ON \
  -DYUN_BUILD_PYTHON=OFF \
  -DCMAKE_C_COMPILER=gcc-14 \
  -DCMAKE_CXX_COMPILER=g++-14 \
  -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES="$PWD/cmake/conan.cmake"
cmake --build build/ci --parallel
ctest --test-dir build/ci --output-on-failure
```

### Windows（clang-cl）

前置条件：

1. Visual Studio 2022 Build Tools（C++ 工作负载 + Windows SDK）——只提供 MSVC STL / 链接器 / SDK，不使用 `cl.exe`。
2. LLVM 17+，`clang-cl` 在 PATH 中（GitHub 的 `windows-2022` / `windows-11-arm` runner 已自带；CI 仅在缺失时用 `choco install llvm` 补齐）。
3. CMake ≥ 3.24、Ninja、Python 3.11、Conan 2.33+。

在 VS 开发者环境（或 `ilammy/msvc-dev-cmd` 提供的环境）中：

```bat
cmake -S . -B build/ci -G Ninja ^
  -DCMAKE_BUILD_TYPE=Debug ^
  -DYUN_BUILD_TESTS=ON ^
  -DYUN_BUILD_PYTHON=OFF ^
  -DCMAKE_C_COMPILER=clang-cl ^
  -DCMAKE_CXX_COMPILER=clang-cl ^
  -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES="%CD%/cmake/conan.cmake"
cmake --build build/ci --parallel
ctest --test-dir build/ci --output-on-failure
```

Windows 只支持 64 位 + 动态 MSVC 运行时（`/MD` 或 `/MDd`）。

## 功能测试

```bash
python scripts/ci/run_functional_case.py \
  --build-dir build/ci \
  --case-zip .ci/yun-cases/feature_tests/C_SUC_001.zip \
  --work-dir build/functional
```

脚本会在任意平台自动查找 `vrp_exec` / `vrp_exec.exe`，解压用例并校验 4 个输出 JSON。

CI 的 5 个 `Build & Test` job 都会执行该用例，工作目录为 `$RUNNER_TEMP/yun-case`；失败时会把 `LastTest.log` 和用例输出目录下的 `*.log` 上传为 artifact（保留 7 天）。脚本用 `pathlib` 解析路径，在 Windows 上自动选择 `vrp_exec.exe`，所以在 `shell: bash` 的 step 里调用也能正常工作。

## 本地验证 Linux arm64（Docker Desktop）

Apple Silicon + Docker Desktop 可以原生运行 `linux/arm64` 容器：

```bash
docker run --rm --platform linux/arm64 \
  -v "$PWD:/src:ro" \
  -v "/path/to/C_SUC_001.zip:/case.zip:ro" \
  quay.io/pypa/manylinux_2_28_aarch64:latest bash -lc '
    source /opt/rh/gcc-toolset-14/enable
    cp -a /src /tmp/src
    /opt/python/cp311-cp311/bin/python -m pip install -q "conan>=2.33,<3"
    export PATH="$(dirname /opt/python/cp311-cp311/bin/python):$PATH"
    export CONAN_HOME=/tmp/conan
    conan profile detect --force
    cmake -S /tmp/src -B /tmp/build -G "Unix Makefiles" \
      -DCMAKE_BUILD_TYPE=Debug -DYUN_BUILD_TESTS=ON -DYUN_BUILD_PYTHON=OFF \
      -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ \
      -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=/tmp/src/cmake/conan.cmake \
      "-DCONAN_INSTALL_ARGS=--build=missing;--lockfile=/tmp/src/conan.lock"
    cmake --build /tmp/build --parallel 2
    ctest --test-dir /tmp/build --output-on-failure --timeout 300
    /opt/python/cp311-cp311/bin/python /tmp/src/scripts/ci/run_functional_case.py \
      --build-dir /tmp/build --case-zip /case.zip --work-dir /tmp/yun-case
  '
```

说明：

- Conan 会写源码根目录的 `CMakeUserPresets.json`，所以容器里先 `cp -a /src /tmp/src`，不要只读挂载后直接构建。
- Docker Desktop VM 内存有限；`--parallel 2` 可避免 `cc1plus` 被 OOM kill。
- 该镜像适合同步验证后续 Linux arm64 SDK 的 manylinux 基线。
- Docker Desktop 不能运行 Windows 容器；Windows ARM64 需要 `windows-11-arm` runner 或 Windows ARM64 VM。

## 已知限制

- 不支持 MSVC 编译器（`cl.exe`）。
- Windows 暂不支持 ASan；CI 的 ASan 只覆盖 macOS 与 Linux x86_64/arm64。
- Windows ARM64 为实验性支持：CI 已通过，但 `windows-11-arm` 仍是 public preview，job 暂保留 `continue-on-error`，不阻断合并。
- 不支持 32 位 Windows。
- Python 预编译 SDK 与 pybind11 扩展仍只发布 macOS arm64（`scripts/build_sdk.py`、`scripts/build_python_extension.py` 里有 `platform.system() == "Darwin"` 检查）；多平台 SDK 需要单独改造这两个脚本和 `sdk-release.yml`。
- Windows 仅支持 clang-cl，不支持 MinGW GCC。
