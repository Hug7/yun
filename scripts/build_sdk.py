#!/usr/bin/env python3
"""构建 yun 的预编译 SDK。

SDK 内容：
  include/             yun 各组件头文件 + conan 依赖头文件（fmt/spdlog/yaml-cpp/...）
  lib/libyun_sdk.a     单体静态库（yun_* + spdlog/fmt/yaml-cpp/lua）
  src/                 yun_solver_binding.cpp
  CMakeLists.txt       消费方用 pybind11 构建 _core 的入口
  VERSION

产物：dist/sdk/yun-sdk-<version>-macos-<arch>.tar.gz + SHA256SUMS
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import re
import shlex
import shutil
import subprocess
import sys
import tarfile
from pathlib import Path

YUN_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_BUILD_DIR = YUN_ROOT / "build" / "sdk"
DEFAULT_OUT_DIR = YUN_ROOT / "dist" / "sdk"
BINDING_SOURCE = YUN_ROOT / "python" / "yun_solver_binding.cpp"
SDK_CMAKE = YUN_ROOT / "python" / "sdk" / "CMakeLists.txt"
TARGET = "yun_python_binding"


def _run(cmd: list[str], *, cwd: Path | None = None) -> None:
    print("+", " ".join(str(part) for part in cmd), flush=True)
    subprocess.run(cmd, cwd=cwd, check=True)


def _require_tool(name: str, hint: str) -> str:
    path = shutil.which(name)
    if path is None:
        raise SystemExit(f"找不到 {name}。{hint}")
    return path


def _project_version() -> str:
    text = (YUN_ROOT / "CMakeLists.txt").read_text()
    match = re.search(r"project\(\s*yun\s+VERSION\s+([0-9.]+)", text)
    if not match:
        raise SystemExit("无法从 CMakeLists.txt 读取 project(yun VERSION ...)")
    return match.group(1)


def _python_info(python: Path) -> tuple[int, int, str]:
    code = (
        "import sys, sysconfig;"
        "print(sys.version_info.major);"
        "print(sys.version_info.minor);"
        "print(sysconfig.get_platform())"
    )
    output = subprocess.check_output([str(python), "-c", code], text=True).splitlines()
    return int(output[0]), int(output[1]), output[2]


def _ensure_conan_profile(conan: str) -> None:
    result = subprocess.run(
        [conan, "profile", "path", "default"],
        capture_output=True,
        text=True,
    )
    if result.returncode == 0:
        return
    print("未找到 conan default profile，执行 conan profile detect --force", flush=True)
    _run([conan, "profile", "detect", "--force"])


def _binding_compile_command(build_dir: Path) -> list[str]:
    data = json.loads((build_dir / "compile_commands.json").read_text())
    for entry in data:
        if not entry["file"].endswith("yun_solver_binding.cpp"):
            continue
        if "arguments" in entry:
            return list(entry["arguments"])
        return shlex.split(entry["command"])
    raise SystemExit("compile_commands.json 中找不到 yun_solver_binding.cpp")


def _include_dirs(command: list[str], directory: Path) -> tuple[list[Path], list[Path]]:
    yun_includes: list[Path] = []
    vendor_includes: list[Path] = []

    def resolve(raw: str) -> Path:
        path = Path(raw)
        return path if path.is_absolute() else (directory / path)

    index = 0
    while index < len(command):
        token = command[index]
        if token == "-I" and index + 1 < len(command):
            yun_includes.append(resolve(command[index + 1]))
            index += 2
            continue
        if token.startswith("-I") and len(token) > 2:
            yun_includes.append(resolve(token[2:]))
            index += 1
            continue
        if token == "-isystem" and index + 1 < len(command):
            vendor_includes.append(resolve(command[index + 1]))
            index += 2
            continue
        if token.startswith("-isystem") and len(token) > len("-isystem"):
            vendor_includes.append(resolve(token[len("-isystem") :].lstrip("=")))
            index += 1
            continue
        index += 1
    return yun_includes, vendor_includes


def _is_skippable_include(path: Path) -> bool:
    text = path.as_posix()
    if path.name.startswith("python") or "/python3" in text:
        return True
    if text.startswith(("/usr/", "/Library/", "/Applications/", "/System/")):
        return True
    # 消费方通过 pip pybind11 提供，不打进 SDK
    return (path / "pybind11").is_dir()


def _link_archives(build_dir: Path) -> list[Path]:
    link_txt = build_dir / "python" / "CMakeFiles" / f"{TARGET}.dir" / "link.txt"
    if not link_txt.is_file():
        raise SystemExit(f"找不到链接脚本: {link_txt}")
    command = shlex.split(link_txt.read_text())
    base = build_dir / "python"
    archives: list[Path] = []
    seen: set[Path] = set()
    for token in command:
        if not token.endswith(".a"):
            continue
        path = Path(token)
        if not path.is_absolute():
            path = (base / path).resolve()
        if path in seen:
            continue
        if not path.is_file():
            raise SystemExit(f"静态库不存在: {path}")
        seen.add(path)
        archives.append(path)
    if not archives:
        raise SystemExit("link.txt 中没有找到 .a 静态库")
    return archives


def _stage_sdk(
    build_dir: Path,
    sdk_root: Path,
    python: Path,
    version: str,
    arch: str,
) -> None:
    if sdk_root.exists():
        shutil.rmtree(sdk_root)
    (sdk_root / "include").mkdir(parents=True)
    (sdk_root / "lib").mkdir()
    (sdk_root / "src").mkdir()

    entry = next(
        item
        for item in json.loads((build_dir / "compile_commands.json").read_text())
        if item["file"].endswith("yun_solver_binding.cpp")
    )
    directory = Path(entry.get("directory", YUN_ROOT))
    yun_includes, vendor_includes = _include_dirs(
        _binding_compile_command(build_dir), directory
    )

    for include in yun_includes:
        include = include.resolve()
        if YUN_ROOT not in include.parents:
            continue
        component = include.parent.name
        destination = sdk_root / "include" / "yun" / component
        shutil.copytree(include, destination, dirs_exist_ok=True)

    for include in vendor_includes:
        include = include.resolve()
        if _is_skippable_include(include) or not include.is_dir():
            continue
        shutil.copytree(include, sdk_root / "include", dirs_exist_ok=True)

    archives = _link_archives(build_dir)
    _run(
        [
            "libtool",
            "-static",
            "-o",
            str(sdk_root / "lib" / "libyun_sdk.a"),
            *[str(archive) for archive in archives],
        ]
    )

    shutil.copy2(BINDING_SOURCE, sdk_root / "src" / "yun_solver_binding.cpp")
    shutil.copy2(SDK_CMAKE, sdk_root / "CMakeLists.txt")
    (sdk_root / "VERSION").write_text(f"{version}\n")
    (sdk_root / "SDK_PLATFORM").write_text(f"macos-{arch}\n")
    print(f"SDK staged: {sdk_root}", flush=True)
    print(f"  python used for link info: {python}", flush=True)


def _make_tar(sdk_root: Path, out_dir: Path, version: str, arch: str) -> Path:
    out_dir.mkdir(parents=True, exist_ok=True)
    name = f"yun-sdk-{version}-macos-{arch}.tar.gz"
    target = out_dir / name
    with tarfile.open(target, "w:gz") as archive:
        for path in sorted(sdk_root.rglob("*")):
            archive.add(path, arcname=path.relative_to(sdk_root))
    digest = hashlib.sha256(target.read_bytes()).hexdigest()
    (out_dir / "SHA256SUMS").write_text(f"{digest}  {name}\n")
    print(f"SDK tarball: {target}", flush=True)
    print(f"sha256: {digest}", flush=True)
    return target


def main() -> None:
    if platform.system() != "Darwin":
        raise SystemExit("当前只支持 macOS")

    parser = argparse.ArgumentParser(description="构建 yun 预编译 SDK")
    parser.add_argument(
        "--version", default=None, help="SDK 版本（默认取 CMake project 版本）"
    )
    parser.add_argument("--out-dir", type=Path, default=DEFAULT_OUT_DIR)
    parser.add_argument("--build-dir", type=Path, default=DEFAULT_BUILD_DIR)
    parser.add_argument("--deployment-target", default="14.0")
    parser.add_argument("--python", default=sys.executable)
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 1)
    parser.add_argument("--clean", action="store_true")
    args = parser.parse_args()

    cmake = _require_tool("cmake", "brew install cmake")
    conan = _require_tool("conan", "python -m pip install 'conan>=2.33,<3'")
    _require_tool("libtool", "libtool 应随 Xcode Command Line Tools 提供")
    _ensure_conan_profile(conan)

    python = Path(args.python).expanduser()
    if not python.is_file():
        resolved = shutil.which(args.python)
        if not resolved:
            raise SystemExit(f"找不到 Python：{args.python}")
        python = Path(resolved)
    python = python.resolve()
    major, minor, platform_tag = _python_info(python)
    if (major, minor) < (3, 9):
        raise SystemExit(f"SDK 构建需要 Python >= 3.9（当前 {major}.{minor}）")

    version = args.version or _project_version()
    arch = platform.machine()
    if arch not in {"arm64", "x86_64"}:
        raise SystemExit(f"不支持的架构: {arch}")

    build_dir = args.build_dir.expanduser().resolve()
    if args.clean and build_dir.exists():
        print(f"清理构建目录 {build_dir}", flush=True)
        shutil.rmtree(build_dir)

    print(
        f"SDK version: {version} ({arch}, deployment {args.deployment_target})",
        flush=True,
    )
    print(f"build python: {python} ({major}.{minor}, {platform_tag})", flush=True)

    _run(
        [
            cmake,
            "-S",
            str(YUN_ROOT),
            "-B",
            str(build_dir),
            "-DCMAKE_BUILD_TYPE=Release",
            "-DYUN_BUILD_PYTHON=ON",
            "-DYUN_BUILD_TESTS=OFF",
            f"-DCMAKE_PROJECT_TOP_LEVEL_INCLUDES={YUN_ROOT / 'conan_provider.cmake'}",
            "-DCONAN_HOST_PROFILE=default",
            f"-DPython3_EXECUTABLE={python}",
            "-DCMAKE_SKIP_RPATH=ON",
            f"-DCMAKE_OSX_DEPLOYMENT_TARGET={args.deployment_target}",
        ]
    )
    _run([cmake, "--build", str(build_dir), "--target", TARGET, "-j", str(args.jobs)])

    sdk_root = build_dir / f"yun-sdk-{version}-macos-{arch}"
    _stage_sdk(build_dir, sdk_root, python, version, arch)
    _make_tar(sdk_root, args.out_dir.expanduser().resolve(), version, arch)


if __name__ == "__main__":
    main()
