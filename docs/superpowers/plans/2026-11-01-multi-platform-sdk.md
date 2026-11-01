# 多平台预编译 SDK 实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 让 `yun` 产出 macOS arm64 / Linux x86_64 / Linux arm64 / Windows x86_64 四平台预编译 SDK，并提供 `find_package(yun)` + `yun::sdk` 消费契约。

**Architecture:** CMake 在配置/生成阶段输出归档与头文件清单，`scripts/build_sdk.py` 读取清单后在每个平台合并静态库、生成 CMake package 并打 tarball；`sdk-release.yml` 用矩阵在对应 runner/容器里构建并发布。

**Tech Stack:** C++20、CMake ≥ 3.24、Conan 2.33+、GitHub Actions、GCC 14、AppleClang、clang-cl、manylinux_2_28、pybind11（仅消费方使用）。

**Spec:** `docs/superpowers/specs/2026-11-01-multi-platform-sdk-design.md`

## Global Constraints

- 平台只做 4 个：`macos-arm64`、`linux-x86_64`、`linux-arm64`、`windows-x86_64`。
- Windows 消费方仅 clang-cl；不做 MSVC、Windows arm64、macOS Intel。
- Linux 基线 manylinux_2_28（glibc ≥ 2.28），GCC 14。
- 不改 `yunpyopt`；不新增 SDK CI；不做发布前消费自检。
- SDK 版本与 `project(yun VERSION ...)` 一致，本轮升到 `0.2.0`。
- 产物统一 `yun-sdk-<version>-<tag>.tar.gz`，tarball 扁平布局。
- 不新增第三方 Python 依赖；脚本只用标准库。

---

## 文件结构

**新增**

- `cmake/sdk.cmake`：生成 SDK 归档/头文件清单。
- `cmake/sdk/CMakeLists.txt`：SDK 根项目，构建 `yunpyopt._core`。
- `cmake/sdk/yunConfig.cmake.in`：`yun::sdk` 导入目标模板。
- `cmake/sdk/yunConfigVersion.cmake.in`：same-minor 版本兼容模板。
- `docs/sdk_guide.md`：SDK 使用与发布说明。

**修改**

- `CMakeLists.txt`：新增 `YUN_BUILD_SDK`，引入 `cmake/sdk.cmake`，版本 0.2.0。
- `scripts/build_sdk.py`：重写为四平台通用。
- `.github/workflows/sdk-release.yml`：矩阵构建 + 合并校验和。
- `conanfile.py`：版本 0.2.0。
- `README.md`、`docs/platform_support.md`：多平台 SDK 说明。

**删除**

- `python/sdk/CMakeLists.txt`（被 `cmake/sdk/CMakeLists.txt` 取代）。

---

### Task 1: CMake 生成 SDK 清单

**Files:**
- Modify: `CMakeLists.txt:92-106`
- Create: `cmake/sdk.cmake`

**Interfaces:**
- Produces: 构建目录下 `sdk-archives-<CONFIG>.txt`（每行一个绝对路径）与 `sdk-includes-<CONFIG>.txt`（每行一个绝对路径）。
- Produces: CMake option `YUN_BUILD_SDK`（默认 OFF）。

- [ ] **Step 1: 写 `cmake/sdk.cmake`**

内容要求（完整实现）：

```cmake
# SDK 打包清单：仅供 scripts/build_sdk.py 读取。
# 不使用 link.txt（Ninja/Windows 下不存在），也不要求构建 Python 绑定。

set(YUN_SDK_OWN_STATIC_TARGETS yun_solver yun_algorithm yun_problem yun_scenario)
set(YUN_SDK_INCLUDE_TARGETS
    fmt::fmt spdlog::spdlog yaml-cpp::yaml-cpp
    magic_enum::magic_enum sol2::sol2 libfork::libfork lua::lua)

foreach(_target IN LISTS YUN_SDK_OWN_STATIC_TARGETS YUN_SDK_INCLUDE_TARGETS)
    if(NOT TARGET ${_target})
        message(FATAL_ERROR "YUN_BUILD_SDK: 找不到目标 ${_target}")
    endif()
endforeach()

# 展开 CMake 的 LINK_ONLY / CONFIG 包装，得到真正的目标名。
function(_yun_unwrap_genex out value)
    set(_value "${value}")
    while(TRUE)
        if(_value MATCHES "^\\$<LINK_ONLY:(.*)>$")
            set(_value "${CMAKE_MATCH_1}")
        elseif(_value MATCHES "^\\$<\\$<CONFIG:Release>:(.*)>$")
            set(_value "${CMAKE_MATCH_1}")
        elseif(_value MATCHES "^\\$<\\$<CONFIG:Debug>:(.*)>$")
            set(_value "")
        else()
            break()
        endif()
    endwhile()
    set(${out} "${_value}" PARENT_SCOPE)
endfunction()

# 从 target 的链接闭包里收集 Conan 静态库的绝对路径。
function(_yun_collect_dependency_archives out)
    set(_queue ${ARGN})
    set(_seen "")
    set(_archives "")
    while(_queue)
        list(POP_FRONT _queue _target)
        if(_target IN_LIST _seen)
            continue()
        endif()
        list(APPEND _seen "${_target}")
        if(NOT TARGET "${_target}")
            continue()
        endif()
        get_target_property(_type "${_target}" TYPE)
        if(_type STREQUAL "STATIC_LIBRARY")
            get_target_property(_location "${_target}" IMPORTED_LOCATION_RELEASE)
            if(NOT _location)
                get_target_property(_location "${_target}" IMPORTED_LOCATION)
            endif()
            if(_location AND NOT _location MATCHES "-NOTFOUND$")
                list(APPEND _archives "${_location}")
            endif()
        endif()
        foreach(_property LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
            get_target_property(_links "${_target}" ${_property})
            if(NOT _links)
                continue()
            endif()
            foreach(_link IN LISTS _links)
                _yun_unwrap_genex(_link "${_link}")
                if(_link)
                    list(APPEND _queue "${_link}")
                endif()
            endforeach()
        endforeach()
    endwhile()
    list(REMOVE_DUPLICATES _archives)
    set(${out} "${_archives}" PARENT_SCOPE)
endfunction()

_yun_collect_dependency_archives(YUN_SDK_DEP_ARCHIVES yun_solver)

set(_own_archives "")
foreach(_target IN LISTS YUN_SDK_OWN_STATIC_TARGETS)
    string(APPEND _own_archives "$<TARGET_FILE:${_target}>\n")
endforeach()
set(_dep_archives "")
foreach(_archive IN LISTS YUN_SDK_DEP_ARCHIVES)
    string(APPEND _dep_archives "${_archive}\n")
endforeach()
file(GENERATE
    OUTPUT "${CMAKE_BINARY_DIR}/sdk-archives-$<CONFIG>.txt"
    CONTENT "${_own_archives}${_dep_archives}")

set(_includes "")
foreach(_target IN LISTS YUN_SDK_INCLUDE_TARGETS)
    string(APPEND _includes
        "$<JOIN:$<TARGET_PROPERTY:${_target},INTERFACE_INCLUDE_DIRECTORIES>,\n>\n")
endforeach()
foreach(_component common scenario problem algorithm solver)
    string(APPEND _includes "${PROJECT_SOURCE_DIR}/${_component}/include\n")
endforeach()
file(GENERATE
    OUTPUT "${CMAKE_BINARY_DIR}/sdk-includes-$<CONFIG>.txt"
    CONTENT "${_includes}")
```

- [ ] **Step 2: 根 `CMakeLists.txt` 接入**

在 `add_subdirectory(solver)` 之后、`option(YUN_BUILD_PYTHON ...)` 之前加入：

```cmake
option(YUN_BUILD_SDK "Generate SDK packaging manifests" OFF)
if(YUN_BUILD_SDK)
    include(cmake/sdk.cmake)
endif()
```

- [ ] **Step 3: 验证清单生成**

Run:

```bash
cmake -S . -B /tmp/yun-sdk-manifest -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DYUN_BUILD_SDK=ON \
  -DYUN_BUILD_TESTS=OFF -DYUN_BUILD_PYTHON=OFF \
  -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES="$PWD/cmake/conan.cmake" \
  "-DCONAN_INSTALL_ARGS=--build=missing;--lockfile=$PWD/conan.lock"
cat /tmp/yun-sdk-manifest/sdk-archives-Release.txt
cat /tmp/yun-sdk-manifest/sdk-includes-Release.txt
```

Expected: 归档清单包含 4 个 `libyun_*.a` + fmt/spdlog/yaml-cpp/lua；头文件清单包含 5 个 yun 组件目录与 7 个 vendored 目录。

- [ ] **Step 4: 提交**

```bash
git add CMakeLists.txt cmake/sdk.cmake
git commit -m "build: generate SDK archive/include manifests from CMake"
```

---

### Task 2: SDK 消费模板

**Files:**
- Create: `cmake/sdk/CMakeLists.txt`
- Create: `cmake/sdk/yunConfig.cmake.in`
- Create: `cmake/sdk/yunConfigVersion.cmake.in`
- Delete: `python/sdk/CMakeLists.txt`

**Interfaces:**
- Produces: `yun::sdk` imported target（静态库、include、编译定义、系统库）。
- Produces: SDK 根项目目标 `yun_python_binding`，输出 `_core`。

- [ ] **Step 1: 写 `cmake/sdk/CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.24)
project(yun_python_binding LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(yun CONFIG REQUIRED HINTS "${CMAKE_CURRENT_SOURCE_DIR}")
find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)
find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(yun_python_binding WITH_SOABI
    "${CMAKE_CURRENT_SOURCE_DIR}/src/yun_solver_binding.cpp")
target_link_libraries(yun_python_binding PRIVATE yun::sdk)
set_target_properties(yun_python_binding PROPERTIES
    OUTPUT_NAME "_core"
    INSTALL_RPATH ""
)
```

- [ ] **Step 2: 写 `cmake/sdk/yunConfig.cmake.in`**

```cmake
# 由 yun scripts/build_sdk.py 生成，请勿手工修改。
include(CMakeFindDependencyMacro)
find_dependency(Threads)

get_filename_component(_yun_sdk_root "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

if(NOT EXISTS "${_yun_sdk_root}/lib/@YUN_SDK_LIB_NAME@")
    set(yun_FOUND FALSE)
    set(yun_NOT_FOUND_MESSAGE
        "yun SDK 缺少 lib/@YUN_SDK_LIB_NAME@（SDK 根目录: ${_yun_sdk_root}）")
    return()
endif()

if(NOT TARGET yun::sdk)
    add_library(yun::sdk STATIC IMPORTED)
    set_target_properties(yun::sdk PROPERTIES
        IMPORTED_LOCATION "${_yun_sdk_root}/lib/@YUN_SDK_LIB_NAME@"
        INTERFACE_INCLUDE_DIRECTORIES
            "${_yun_sdk_root}/include;${_yun_sdk_root}/include/yun/common;${_yun_sdk_root}/include/yun/scenario;${_yun_sdk_root}/include/yun/problem;${_yun_sdk_root}/include/yun/algorithm;${_yun_sdk_root}/include/yun/solver"
        INTERFACE_COMPILE_FEATURES "cxx_std_20"
        INTERFACE_COMPILE_DEFINITIONS
            "SPDLOG_COMPILED_LIB;SPDLOG_FMT_EXTERNAL;YAML_CPP_STATIC_DEFINE"
        INTERFACE_LINK_LIBRARIES "@YUN_SDK_SYSTEM_LIBS@"
    )
endif()

set(yun_FOUND TRUE)
unset(_yun_sdk_root)
```

- [ ] **Step 3: 写 `cmake/sdk/yunConfigVersion.cmake.in`**

```cmake
set(PACKAGE_VERSION "@YUN_VERSION@")

if(PACKAGE_FIND_VERSION)
    if(PACKAGE_FIND_VERSION VERSION_GREATER PACKAGE_VERSION)
        set(PACKAGE_VERSION_COMPATIBLE FALSE)
    else()
        set(PACKAGE_VERSION_COMPATIBLE TRUE)
    endif()
    # 0.x：minor 是 ABI 兼容边界
    if(PACKAGE_FIND_VERSION_MAJOR EQUAL 0
       AND NOT PACKAGE_FIND_VERSION_MINOR EQUAL ${PACKAGE_VERSION_MINOR})
        set(PACKAGE_VERSION_COMPATIBLE FALSE)
    endif()
    if(PACKAGE_FIND_VERSION VERSION_EQUAL PACKAGE_VERSION)
        set(PACKAGE_VERSION_EXACT TRUE)
    endif()
endif()
```

- [ ] **Step 4: 删除旧模板**

```bash
git rm python/sdk/CMakeLists.txt
```

- [ ] **Step 5: 提交**

```bash
git add cmake/sdk
git commit -m "feat: add yun SDK CMake package templates"
```

---

### Task 3: 重写 `scripts/build_sdk.py`

**Files:**
- Modify: `scripts/build_sdk.py`（整文件替换）

**Interfaces:**
- CLI：`--version`、`--out-dir`、`--build-dir`、`--deployment-target`、`--jobs`、`--clean`。
- Produces：`dist/yun-sdk-<version>-<tag>.tar.gz` 与同名 `.sha256`。

- [ ] **Step 1: 实现平台检测与标签**

```python
def _platform_tag() -> str:
    system = platform.system()
    machine = platform.machine().lower()
    if system == "Darwin" and machine == "arm64":
        return "macos-arm64"
    if system == "Linux" and machine in {"x86_64", "amd64"}:
        return "linux-x86_64"
    if system == "Linux" and machine in {"aarch64", "arm64"}:
        return "linux-arm64"
    if system == "Windows" and machine in {"amd64", "x86_64"}:
        return "windows-x86_64"
    raise SystemExit(f"不支持的平台: {system}/{machine}")
```

- [ ] **Step 2: 实现清单读取与归档合并**

- 读 `build_dir/sdk-archives-Release.txt`、`build_dir/sdk-includes-Release.txt`；
- 去空行、去重、`Path.resolve()`；
- macOS：`libtool -static -o libyun_sdk.a <archives...>`；
- Linux：`ar -M`，脚本 `CREATE /abs/libyun_sdk.a` + 每行 `ADDLIB <abs>` + `SAVE` + `END`，通过 stdin 传入；
- Windows：`llvm-lib /nologo /OUT:<abs> <archives...>`，找不到时回退 `lib`；
- 任一归档缺失时报错退出。

- [ ] **Step 3: 实现 staging**

- `include/yun/<component>/`：拷贝 5 个 yun 组件 include 目录；
- `include/`：合并 7 个 vendored include 目录；
- `lib/<lib_name>`：合并后的静态库；
- `lib/cmake/yun/yunConfig.cmake`：替换 `@YUN_SDK_LIB_NAME@`、`@YUN_SDK_SYSTEM_LIBS@`、`@YUN_VERSION@`；
- `lib/cmake/yun/yunConfigVersion.cmake`：替换 `@YUN_VERSION@`；
- `src/yun_solver_binding.cpp`、`CMakeLists.txt`、`VERSION`、`SDK_PLATFORM`、`LICENSE`；
- 文本文件统一 UTF-8 + `\n` 换行。

- [ ] **Step 4: 实现 CMake 配置参数**

- 公共：`CMAKE_BUILD_TYPE=Release`、`YUN_BUILD_SDK=ON`、`YUN_BUILD_TESTS=OFF`、`YUN_BUILD_PYTHON=OFF`、`CMAKE_SKIP_RPATH=ON`、`CMAKE_PROJECT_TOP_LEVEL_INCLUDES=cmake/conan.cmake`、`CONAN_INSTALL_ARGS=--build=missing;--lockfile=...`；
- macOS：`CMAKE_OSX_ARCHITECTURES=arm64`、`CMAKE_OSX_DEPLOYMENT_TARGET=14.0`；
- Linux：`CMAKE_POSITION_INDEPENDENT_CODE=ON`；
- Windows：`-G Ninja`、`CMAKE_C_COMPILER=clang-cl`、`CMAKE_CXX_COMPILER=clang-cl`。

- [ ] **Step 5: 本机 macOS 验证**

Run:

```bash
python3.11 scripts/build_sdk.py --version 0.2.0 --out-dir /tmp/yun-sdk --clean
tar -tzf /tmp/yun-sdk/yun-sdk-0.2.0-macos-arm64.tar.gz
```

Expected: 包含 `lib/libyun_sdk.a`、`lib/cmake/yun/yunConfig.cmake`、`VERSION`、`SDK_PLATFORM`，且没有 pybind11/Python 头文件。

- [ ] **Step 6: 提交**

```bash
git add scripts/build_sdk.py
git commit -m "build: make build_sdk platform-generic"
```

---

### Task 4: 发布矩阵

**Files:**
- Modify: `.github/workflows/sdk-release.yml`

**Interfaces:**
- Job 名：`version`、`build-macos`、`build-linux`、`build-windows`、`release`。
- Artifact 名：`yun-sdk-macos-arm64`、`yun-sdk-linux-x86_64`、`yun-sdk-linux-arm64`、`yun-sdk-windows-x86_64`。

- [ ] **Step 1: 重写 workflow**

要求：

- `version` job 输出 `steps.resolve.outputs.version`；
- `build-linux` 用 `container: quay.io/pypa/manylinux_2_28_{x86_64,aarch64}`，步骤里 `source /opt/rh/gcc-toolset-14/enable`，用 `/opt/python/cp311-cp311/bin/python` 安装并运行 Conan；
- `build-windows` 用 `ilammy/msvc-dev-cmd@v1`、安装 LLVM/Ninja、`python -m pip install "conan>=2.33,<3"`；
- 每个 build job 上传 `dist/*.tar.gz`；
- `release` job 下载 `yun-sdk-*`，`sha256sum *.tar.gz | sort -k2 > SHA256SUMS`，tag 触发时用 `softprops/action-gh-release@v3` 发布；
- 保留 `environment: release` 审批。

- [ ] **Step 2: YAML 校验**

Run:

```bash
python -c "import yaml, pathlib; yaml.safe_load(pathlib.Path('.github/workflows/sdk-release.yml').read_text()); print('ok')"
```

Expected: `ok`。

- [ ] **Step 3: 提交**

```bash
git add .github/workflows/sdk-release.yml
git commit -m "ci: build multi-platform SDK in release workflow"
```

---

### Task 5: 版本与文档

**Files:**
- Modify: `CMakeLists.txt:4`
- Modify: `conanfile.py:13`
- Create: `docs/sdk_guide.md`
- Modify: `README.md:213-260`
- Modify: `docs/platform_support.md:114-121`

- [ ] **Step 1: 版本升到 0.2.0**

- `project(yun VERSION 0.2.0 LANGUAGES C CXX)`
- `version = "0.2.0"`

- [ ] **Step 2: 写 `docs/sdk_guide.md`**

覆盖：平台矩阵、构建环境、SDK 内容、`find_package` + `yun::sdk` 用法、pybind11 消费示例、ABI/编译器约束、发布流程、验证责任在 yunpyopt。

- [ ] **Step 3: 更新 README 与 platform_support**

- README §5 改为多平台 SDK 说明，并链接 `docs/sdk_guide.md`；
- `docs/platform_support.md` 的已知限制改为四平台 SDK + Windows 仅 clang-cl 消费方。

- [ ] **Step 4: 提交**

```bash
git add CMakeLists.txt conanfile.py docs/sdk_guide.md README.md docs/platform_support.md
git commit -m "docs: document multi-platform SDK and bump version to 0.2.0"
```

---

### Task 6: 最终验证

- [ ] **Step 1: Python 质量门禁**

```bash
ruff check scripts/build_sdk.py
ruff format --check scripts/build_sdk.py
```

- [ ] **Step 2: macOS SDK + 消费方**

```bash
python3.11 scripts/build_sdk.py --version 0.2.0 --out-dir /tmp/yun-sdk --clean
mkdir -p /tmp/yun-sdk/use && tar -xzf /tmp/yun-sdk/yun-sdk-0.2.0-macos-arm64.tar.gz -C /tmp/yun-sdk/use
cmake -S /tmp/yun-sdk/use -B /tmp/yun-sdk/build \
  -DCMAKE_BUILD_TYPE=Release \
  -DPython3_EXECUTABLE="$(which python3.11)" \
  -Dpybind11_DIR="$(python3.11 -c 'import pybind11; print(pybind11.get_cmake_dir())')"
cmake --build /tmp/yun-sdk/build --parallel
python3.11 -c "import sys; sys.path.insert(0, '/tmp/yun-sdk/build'); import _core; print(_core.__doc__.splitlines()[0])"
```

- [ ] **Step 3: Linux manylinux arm64（Docker）**

用 `quay.io/pypa/manylinux_2_28_aarch64` 跑 `scripts/build_sdk.py`，检查 tarball 内容；不做跨平台消费验证（交给 yunpyopt）。

- [ ] **Step 4: 手动触发 `sdk-release` workflow_dispatch**

确认 4 个 artifact 生成；Windows/x86_64 由该次构建验证。
