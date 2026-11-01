# 质量门禁

| 门禁 | 工具 | 版本 | CI job |
|---|---|---|---|
| C++ 格式 | clang-format | 22.1.8 | `Lint (ubuntu)` |
| Python lint/格式 | ruff | 0.15.20 | `Lint (ubuntu)` |
| workflow 语法 | actionlint | 1.7.8 | `Lint (ubuntu)` |
| 单测 + 功能测试 | ctest / yun-cases v0.1.0 | - | `Build & Test (macOS arm64 AppleClang)`、`Build & Test (Linux x86_64 GCC 14)`、`Build & Test (Linux arm64 GCC 14)`、`Build & Test (Linux x86_64 Clang 18)`、`Build & Test (Linux arm64 Clang 18)`、`Build & Test (Windows x64 clang-cl)`、`Build & Test (Windows arm64 clang-cl)`（实验性） |
| 内存错误 | AddressSanitizer | Xcode 自带 / GCC 14 | `ASan (macOS arm64)`、`ASan (Linux x86_64 GCC 14)`、`ASan (Linux arm64 GCC 14)` |

平台与工具链最低版本见 [platform_support.md](platform_support.md)。

## 本地命令

```bash
# C++ 格式（repo 根目录）
git ls-files -z '*.h' '*.cpp' | xargs -0 clang-format --dry-run --Werror

# Python
ruff check conanfile.py scripts/
ruff format --check conanfile.py scripts/

# 单测 + 功能测试
cmake --build build/ci --parallel
ctest --test-dir build/ci --output-on-failure --timeout 300

# 功能测试（跨平台 Python 驱动）
python scripts/ci/run_functional_case.py \
  --build-dir build/ci \
  --case-zip .ci/yun-cases/feature_tests/C_SUC_001.zip \
  --work-dir build/functional

# ASan（macOS / Linux）
cmake --build build/asan --parallel
ASAN_OPTIONS=detect_leaks=0:abort_on_error=1 ctest --test-dir build/asan --output-on-failure
```

## 注意

- `common/include/c_rapidcsv.h` 是 vendored 第三方头文件，已在 `.clang-format-ignore` 排除。
- `clang-format` 必须是 22.x；`.clang-format` 的 `AlignFunctionDeclarations` 在 21 及以下会解析失败。
- 领域数值统一使用 `std::int64_t`（显式 `#include <cstdint>`），不要使用平台宽度不一致的 `long`。
- 修改 `conanfile.py` 的依赖后需要重新生成 lockfile：

```bash
conan lock create . -s build_type=Debug -s compiler.cppstd=20 --lockfile-out=conan.lock
```
