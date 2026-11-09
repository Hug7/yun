# 仓库约定

本文规定哪些文件进入版本库、哪些属于本机生成物，以及行尾与二进制文件的处理。

## 1. 必须入库

团队共享、构建或 CI 依赖的内容：

- 构建与依赖：`CMakeLists.txt`、`CMakePresets.json`、`conanfile.py`、`conan.lock`、
  `conan_provider.cmake`、`cmake/`、`scripts/`
- 质量门禁：`.clang-format`、`.clang-format-ignore`、`ruff.toml`
- CI：`.github/workflows/`
- 文档：`README.md`、`AGENTS.md`、`CONTEXT.md`、`docs/`、`CHANGELOG.md`、`VERSION`
- 人读的示例配置：`RUN*.sh`、`.agents/`

## 2. 不入库

本机文件或生成物：

- `.vscode/`、`.idea/`、`.VSCodeCounter/`：含本机路径（如 `.vscode/c_cpp_properties.json`
  的编译器路径），不具备可移植性
- `CMakeUserPresets.json`：Conan 本机生成，用 `RUN_CONAN.sh` 或 `conan install` 重新生成
- `build/`、`cmake-build-*`、`dist/`、`wheelhouse/`、`tmp/`、`log/`、`.ci/`、各类缓存目录
- `.codegraph/`：由目录内自带的 `.gitignore` 忽略
- `docs/superpowers/`：由人工决定是否入库（见 `AGENTS.md`），保持 `git status` 可见，
  不要用 `git add -f` 绕过

## 3. 行尾与二进制

- `.gitattributes` 使用 `* text=auto eol=lf` 统一 LF，Windows 检出也保持 LF，
  避免 clang-format、ruff 产生整文件 diff。
- 图片、压缩包、库文件显式标记为 `binary`。
- 大文件：单个超过约 5 MB 再考虑 Git LFS；当前 `resources/assets/logo.png`
  （约 1.3 MB）直接入库即可。

## 4. 新增文件时

- 先判断属于“团队共享”还是“本机生成物”，不要用 `git add -f` 绕过忽略规则。
- 文档与配置的改动和代码放在同一个 PR，避免只改代码不改文档。
