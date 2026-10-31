# 质量门禁

| 门禁 | 工具 | 版本 | CI job |
|---|---|---|---|
| C++ 格式 | clang-format | 22.1.8 | `Lint (ubuntu)` |
| Python lint/格式 | ruff | 0.15.20 | `Lint (ubuntu)` |
| workflow 语法 | actionlint | 1.7.7 | `Lint (ubuntu)` |
| 单测 + 功能测试 | ctest / yun-cases v0.1.0 | - | `Build & Test (macOS arm64)` |
| 内存错误 | AddressSanitizer | Xcode 自带 | `ASan (macOS arm64)` |

## 本地命令

```bash
# C++ 格式（repo 根目录）
git ls-files -z '*.h' '*.cpp' | xargs -0 clang-format --dry-run --Werror

# Python
ruff check conanfile.py scripts/
ruff format --check conanfile.py scripts/

# 单测 + 功能测试
cmake --build build/ci --parallel "$(sysctl -n hw.ncpu)"
ctest --test-dir build/ci --output-on-failure --timeout 120
YUN_CASE_DIR=/path/to/C_SUC_001 YUN_LOG_DIR=/tmp/yun-log ./build/ci/tests/vrp_exec

# ASan
cmake --build build/asan --parallel "$(sysctl -n hw.ncpu)"
ASAN_OPTIONS=detect_leaks=0:abort_on_error=1 ctest --test-dir build/asan --output-on-failure
```

## 注意

- `common/include/c_rapidcsv.h` 是 vendored 第三方头文件，已在 `.clang-format-ignore` 排除。
- `clang-format` 必须是 22.x；`.clang-format` 的 `AlignFunctionDeclarations` 在 21 及以下会解析失败。
- 修改 `conanfile.py` 的依赖后需要重新生成 lockfile：

```bash
conan lock create . -s build_type=Debug -s compiler.cppstd=20 --lockfile-out=conan.lock
```
