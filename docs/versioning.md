# 版本管理

本文规定 yun 的版本号如何产生、校验与发布；文档与配置的入库规则见
[repository_conventions.md](repository_conventions.md)。

## 1. 版本地址维护

仓库根目录 `VERSION` 保存一行 `X.Y.Z`，这是唯一需要人工修改版本号的地方。

| 位置 | 如何取得版本 | 说明 |
|---|---|---|
| `CMakeLists.txt` | `file(STRINGS .../VERSION)` 后传给 `project(yun VERSION ...)` | 配置阶段打印 `yun version: ...` |
| `scripts/build_sdk.py` | 读取 `VERSION` | `--version` 仅用于显式校验，必须一致 |
| `.github/workflows/sdk-release.yml` | `tr -d '[:space:]' < VERSION` | tag 触发时校验 `tag == v${VERSION}` |
| `conanfile.py` | `version = _project_version()`，运行时读 `VERSION` | 不写死字面量；`conan install .` 是 consumer 流程，recipe 不会导出到缓存 |
| README / `docs/` | 不写死版本 | 命令默认取 `VERSION`；路径写 `<version>` |
| `CHANGELOG.md` | 人工维护 | 只记录已发布或待发布的变更 |

改版本时不要手改多处，统一走第 3 节的脚本。

## 2. 版本规则

- 语义化版本 `MAJOR.MINOR.PATCH`；0.x 阶段 minor 是 ABI 断点，patch 视为兼容。
- 发布 tag 形如 `vX.Y.Z`，必须与 `VERSION` 完全一致；CI 与 release workflow 都会校验。
- tag 是发布动作，不是真源：构建需要静态版本，不能从 tag 反推。
- 不覆盖已发布的 SDK 资产；C++ 变更后按 ABI 情况升 minor 或 patch 重新发布。

## 3. 日常命令

```bash
python scripts/version.py show        # 打印当前版本
python scripts/version.py check       # 校验所有派生位置（CI 会跑）
python scripts/version.py bump patch  # major | minor | patch
python scripts/version.py set 0.4.0
```

`bump` / `set` 会依次：更新 `VERSION` → 把 `CHANGELOG.md` 的
`## [Unreleased]` 转为 `## [x.y.z] - 日期` 并新建空的 Unreleased 段
（若没有 Unreleased 段，则直接在最新版本段之前插入新版本段），
然后打印后续步骤；脚本不会自动 commit、打 tag 或 push。

发布检查清单：

1. `python scripts/version.py bump minor`（或 `patch`）
2. 补全 CHANGELOG 条目，走 PR 合入
3. 合并后人工打 tag：`git tag -a vX.Y.Z -m "yun X.Y.Z"`（推送需批准）
4. push tag 触发 SDK Release；`release` environment 需人工批准

## 4. 文档写法约定

- 构建命令不带 `--version`：`python scripts/build_sdk.py --out-dir dist/sdk --clean`。
- 产物路径用占位符：`yun-sdk-<version>-<platform>.tar.gz`。
- 文档里不写“当前版本是 0.x.y”这类会漂移的表述；当前版本只存在 `VERSION` 里。
- 历史版本只写在 `CHANGELOG.md`；设计、计划文档中的旧版本号不受此限。
- `docs/superpowers/**` 不参与版本检查。

## 5. 常见问题

**conanfile.py 的版本要和项目版本一致吗？**
conanfile 里不写版本字面量，`version = _project_version()` 在加载 recipe 时读取根目录
`VERSION`，所以天然一致，不存在需要同步的副本。

- `conan install .` 是 consumer 流程，recipe 不会导出到缓存、也不发布；此时 `version`
  只用于图根显示（`conanfile.py (yun/<version>)`）。
- 如果将来要 `conan export` / `conan create` 发布这份 recipe，导出到缓存后 recipe 副本
  旁边没有 `VERSION`，再加载会失败；届时需要改为导出时注入版本，或在发布流水线里
  显式写回字面量。当前仓库不发布 recipe，因此不引入这层复杂度。
- `scripts/version.py check` 会校验：conanfile 只有一处 `version` 赋值，且必须来自
  `_project_version()`，不能出现字面量。

**为什么不让 build_sdk.py 继续解析 CMakeLists？**
用正则解析构建脚本会把“CMake 的写法”变成隐式接口；读 `VERSION` 更稳定，
也让版本读取逻辑只存在于 CMake 与脚本各一处。
