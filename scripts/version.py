#!/usr/bin/env python3
"""统一版本管理：读取、校验与升级 yun 的版本号。

版本真源是仓库根目录的 ``VERSION``（单行 ``X.Y.Z``）。CMake、SDK 构建脚本、
release workflow 与 ``conanfile.py`` 都在运行时读取它，仓库里没有第二份版本号。
规则与发布流程见 ``docs/versioning.md``。

用法::

    python scripts/version.py show
    python scripts/version.py check
    python scripts/version.py bump patch
    python scripts/version.py set 0.4.0
"""

from __future__ import annotations

import argparse
import datetime as dt
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import List

ROOT = Path(__file__).resolve().parents[1]
VERSION_FILE = ROOT / "VERSION"
CONAN_FILE = ROOT / "conanfile.py"
CMAKE_FILE = ROOT / "CMakeLists.txt"
RELEASE_WORKFLOW = ROOT / ".github" / "workflows" / "sdk-release.yml"
CHANGELOG = ROOT / "CHANGELOG.md"
DOCS_DIR = ROOT / "docs"

SEMVER_RE = re.compile(r"^[0-9]+\.[0-9]+\.[0-9]+$")
CONAN_VERSION_ASSIGN_RE = re.compile(r"(?m)^[ \t]*version[ \t]*=[ \t]*(.*)$")
CONAN_PROJECT_VERSION_CALL = "_project_version()"
CMAKE_READS_VERSION_RE = re.compile(
    r'file\(STRINGS\s+"\$\{CMAKE_CURRENT_LIST_DIR\}/VERSION"'
)
CMAKE_PROJECT_RE = re.compile(r"project\(\s*yun\s+VERSION\s+\$\{YUN_VERSION\}")
TAG_VERSION_RE = re.compile(r"^v([0-9]+\.[0-9]+\.[0-9]+)$")
UNRELEASED_RE = re.compile(r"^## \[Unreleased\]", re.MULTILINE)
DOC_HARDCODE_RULES = (
    (
        re.compile(r"--version\s+([0-9]+\.[0-9]+\.[0-9]+)"),
        "改用默认值（不带 --version）",
    ),
    (re.compile(r"yun-sdk-([0-9]+\.[0-9]+\.[0-9]+)-"), "改用 yun-sdk-<version>-..."),
)
CHANGELOG_TEMPLATE = """# 更新日志

记录对外可见的变更；版本号格式与发布流程见
[docs/versioning.md](docs/versioning.md)。

## [Unreleased]
"""


class VersionError(RuntimeError):
    """版本文件或派生位置不可用。"""


def read_version() -> str:
    if not VERSION_FILE.is_file():
        raise VersionError(f"缺少版本文件: {VERSION_FILE}")
    version = VERSION_FILE.read_text(encoding="utf-8").strip()
    if not SEMVER_RE.fullmatch(version):
        raise VersionError(f"VERSION 内容非法（应为 X.Y.Z）: {version!r}")
    return version


def write_version(version: str) -> None:
    if not SEMVER_RE.fullmatch(version):
        raise VersionError(f"版本号非法（应为 X.Y.Z）: {version!r}")
    VERSION_FILE.write_text(f"{version}\n", encoding="utf-8")


def _doc_files() -> List[Path]:
    files = [path for path in sorted(ROOT.glob("*.md")) if path.name != CHANGELOG.name]
    files += [
        path
        for path in sorted(DOCS_DIR.rglob("*.md"))
        if "superpowers" not in path.relative_to(DOCS_DIR).parts
    ]
    return [path for path in files if path.is_file()]


def _head_tag() -> str | None:
    try:
        result = subprocess.run(
            ["git", "-C", str(ROOT), "describe", "--tags", "--exact-match", "HEAD"],
            capture_output=True,
            text=True,
            check=False,
        )
    except OSError:
        return None
    if result.returncode != 0:
        return None
    return result.stdout.strip() or None


def check() -> list[str]:
    """返回不一致项；空列表表示通过。"""
    version = read_version()
    problems: list[str] = []

    conan_text = CONAN_FILE.read_text(encoding="utf-8")
    conan_assignments = CONAN_VERSION_ASSIGN_RE.findall(conan_text)
    if len(conan_assignments) != 1:
        problems.append(
            f"conanfile.py: version 赋值出现 {len(conan_assignments)} 次（应为 1 次）"
        )
    elif conan_assignments[0].strip() != CONAN_PROJECT_VERSION_CALL:
        problems.append(
            f"conanfile.py: version 应来自 {CONAN_PROJECT_VERSION_CALL}，不得写死字面量"
        )
    elif "VERSION" not in conan_text:
        problems.append("conanfile.py: 未读取仓库根目录 VERSION")

    cmake = CMAKE_FILE.read_text(encoding="utf-8")
    if not CMAKE_READS_VERSION_RE.search(cmake):
        problems.append("CMakeLists.txt: 未从 VERSION 文件读取版本")
    if not CMAKE_PROJECT_RE.search(cmake):
        problems.append(
            "CMakeLists.txt: project(yun VERSION ...) 未使用 ${YUN_VERSION}"
        )

    workflow = RELEASE_WORKFLOW.read_text(encoding="utf-8")
    if "VERSION" not in workflow:
        problems.append("sdk-release.yml: 未读取 VERSION")
    for hardcoded in re.findall(r"\b[0-9]+\.[0-9]+\.[0-9]+\b", workflow):
        problems.append(f"sdk-release.yml: 硬编码版本 {hardcoded}")

    for path in _doc_files():
        rel = path.relative_to(ROOT)
        for lineno, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            for pattern, hint in DOC_HARDCODE_RULES:
                found = pattern.search(line)
                if found is not None:
                    problems.append(
                        f"{rel}:{lineno}: 硬编码版本 {found.group(1)}（{hint}）"
                    )

    if not CHANGELOG.is_file():
        problems.append("缺少 CHANGELOG.md")
    else:
        changelog = CHANGELOG.read_text(encoding="utf-8")
        has_released = f"## [{version}]" in changelog
        if not has_released and not UNRELEASED_RE.search(changelog):
            problems.append(
                f"CHANGELOG.md: 缺少 '## [{version}]' 或 '## [Unreleased]' 段落"
            )

    github_ref = os.environ.get("GITHUB_REF", "")
    tag = None
    if github_ref.startswith("refs/tags/"):
        tag = os.environ.get("GITHUB_REF_NAME")
    else:
        tag = _head_tag()
    tag_match = TAG_VERSION_RE.fullmatch(tag) if tag else None
    if tag_match is not None and tag_match.group(1) != version:
        problems.append(f"git tag {tag} 与 VERSION {version} 不一致")

    return problems


def next_version(current: str, part: str) -> str:
    major, minor, patch = (int(component) for component in current.split("."))
    if part == "major":
        return f"{major + 1}.0.0"
    if part == "minor":
        return f"{major}.{minor + 1}.0"
    return f"{major}.{minor}.{patch + 1}"


def _roll_changelog_text(text: str, version: str) -> str:
    if f"## [{version}]" in text:
        raise VersionError(f"CHANGELOG.md 已存在 {version} 段落")
    today = dt.date.today().isoformat()
    section = f"## [{version}] - {today}"
    match = UNRELEASED_RE.search(text)
    if match is not None:
        replacement = f"## [Unreleased]\n\n{section}"
        return text[: match.start()] + replacement + text[match.end() :]
    first_section = re.search(r"(?m)^## \[", text)
    if first_section is None:
        return f"{text.rstrip()}\n\n{section}\n"
    return (
        text[: first_section.start()] + f"{section}\n\n" + text[first_section.start() :]
    )


def _release(version: str) -> None:
    if not SEMVER_RE.fullmatch(version):
        raise VersionError(f"版本号非法（应为 X.Y.Z）: {version!r}")
    text = (
        CHANGELOG.read_text(encoding="utf-8")
        if CHANGELOG.is_file()
        else CHANGELOG_TEMPLATE
    )
    updated_changelog = _roll_changelog_text(text, version)
    write_version(version)
    CHANGELOG.write_text(updated_changelog, encoding="utf-8")


def _print_next_steps(version: str) -> None:
    print("后续步骤：")
    print("  1. 审阅并提交 CHANGELOG.md、VERSION")
    print(f'  2. git tag -a v{version} -m "yun {version}"')
    print(f"  3. git push origin main v{version}   # 推送前需人工批准")


def main(argv: List[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="yun 统一版本管理")
    subparsers = parser.add_subparsers(dest="command", required=True)
    subparsers.add_parser("show", help="打印当前版本")
    subparsers.add_parser("check", help="校验所有派生位置（CI 使用）")
    bump = subparsers.add_parser("bump", help="升级版本并更新 CHANGELOG")
    bump.add_argument("part", choices=["major", "minor", "patch"])
    set_parser = subparsers.add_parser("set", help="直接设置版本")
    set_parser.add_argument("version", help="目标版本（X.Y.Z）")
    args = parser.parse_args(argv)

    try:
        if args.command == "show":
            print(read_version())
        elif args.command == "check":
            problems = check()
            for problem in problems:
                print(f"✗ {problem}", file=sys.stderr)
            if problems:
                print(
                    f"\n版本校验失败：{len(problems)} 处不一致；按提示修正后重新运行。",
                    file=sys.stderr,
                )
                return 1
            print(f"版本校验通过：{read_version()}")
        elif args.command == "set":
            _release(args.version)
            print(f"版本已设置为 {args.version}")
            _print_next_steps(args.version)
        elif args.command == "bump":
            current = read_version()
            version = next_version(current, args.part)
            _release(version)
            print(f"版本 {current} -> {version}")
            _print_next_steps(version)
    except VersionError as exc:
        print(f"错误：{exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
