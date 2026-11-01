#!/usr/bin/env python3
"""构建 yun 的预编译 SDK（macOS / Linux / Windows）。

SDK 内容：
  include/             yun 各组件头文件 + vendored 依赖头文件
  lib/libyun_sdk.a     单体静态库（yun_* + spdlog/fmt/yaml-cpp/lua）
                       Windows 下为 lib/yun_sdk.lib
  lib/cmake/yun/       yunConfig.cmake / yunConfigVersion.cmake
  src/                 yun_solver_binding.cpp
  CMakeLists.txt       消费方用 pybind11 构建 _core 的入口
  VERSION
  SDK_PLATFORM
  LICENSE

产物：<out-dir>/yun-sdk-<version>-<platform>.tar.gz + 同名 .sha256
"""

from __future__ import annotations

import argparse
import hashlib
import os
import platform
import re
import shutil
import subprocess
import sys
import tarfile
from pathlib import Path

YUN_ROOT = Path(__file__).resolve().parents[1]
CONAN_LOCK = YUN_ROOT / "conan.lock"
DEFAULT_BUILD_DIR = YUN_ROOT / "build" / "sdk"
DEFAULT_OUT_DIR = YUN_ROOT / "dist" / "sdk"
BINDING_SOURCE = YUN_ROOT / "python" / "yun_solver_binding.cpp"
SDK_TEMPLATE_DIR = YUN_ROOT / "cmake" / "sdk"
SDK_CMAKE = SDK_TEMPLATE_DIR / "CMakeLists.txt"
SDK_CONFIG_TEMPLATE = SDK_TEMPLATE_DIR / "yunConfig.cmake.in"
SDK_VERSION_TEMPLATE = SDK_TEMPLATE_DIR / "yunConfigVersion.cmake.in"
LICENSE_FILE = YUN_ROOT / "LICENSE"
BUILD_TARGET = "yun_solver"
COMPONENTS = ("common", "scenario", "problem", "algorithm", "solver")


def _use_utf8_stdio() -> None:
    """让日志在非 UTF-8 控制台上也安全。

    Windows 的管道流默认使用 ANSI 代码页（如 cp1252），打印中文会抛
    UnicodeEncodeError；这里统一改成 UTF-8，并对无法编码的字符转义。
    """
    for stream in (sys.stdout, sys.stderr):
        reconfigure = getattr(stream, "reconfigure", None)
        if reconfigure is not None:
            reconfigure(encoding="utf-8", errors="backslashreplace")


_use_utf8_stdio()


def _run(cmd: list[str]) -> None:
    print("+", " ".join(str(part) for part in cmd), flush=True)
    subprocess.run(cmd, check=True)


def _require_tool(name: str, hint: str) -> str:
    path = shutil.which(name)
    if path is None:
        raise SystemExit(f"找不到 {name}。{hint}")
    return path


def _project_version() -> str:
    text = (YUN_ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    match = re.search(r"project\(\s*yun\s+VERSION\s+([0-9.]+)", text)
    if not match:
        raise SystemExit("无法从 CMakeLists.txt 读取 project(yun VERSION ...)")
    return match.group(1)


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


def _lib_name(tag: str) -> str:
    return "yun_sdk.lib" if tag.startswith("windows-") else "libyun_sdk.a"


def _system_libs(tag: str) -> str:
    if tag.startswith("linux-"):
        return "Threads::Threads;${CMAKE_DL_LIBS};m"
    return "Threads::Threads"


def _write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="\n") as handle:
        handle.write(text)


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


def _configure_args(
    cmake: str,
    build_dir: Path,
    tag: str,
    deployment_target: str,
) -> list[str]:
    args = [
        cmake,
        "-S",
        YUN_ROOT.as_posix(),
        "-B",
        build_dir.as_posix(),
        "-DCMAKE_BUILD_TYPE=Release",
        "-DYUN_BUILD_SDK=ON",
        "-DYUN_BUILD_TESTS=OFF",
        "-DYUN_BUILD_PYTHON=OFF",
        "-DCMAKE_PROJECT_TOP_LEVEL_INCLUDES="
        f"{(YUN_ROOT / 'cmake' / 'conan.cmake').as_posix()}",
        f"-DCONAN_INSTALL_ARGS=--build=missing;--lockfile={CONAN_LOCK.as_posix()}",
        "-DCMAKE_SKIP_RPATH=ON",
    ]
    if tag.startswith("macos-"):
        args.extend(
            [
                "-DCMAKE_OSX_ARCHITECTURES=arm64",
                f"-DCMAKE_OSX_DEPLOYMENT_TARGET={deployment_target}",
                "-DCMAKE_POSITION_INDEPENDENT_CODE=ON",
            ]
        )
    elif tag.startswith("linux-"):
        args.append("-DCMAKE_POSITION_INDEPENDENT_CODE=ON")
    elif tag.startswith("windows-"):
        args.extend(
            [
                "-G",
                "Ninja",
                "-DCMAKE_C_COMPILER=clang-cl",
                "-DCMAKE_CXX_COMPILER=clang-cl",
                "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL",
            ]
        )
    return args


def _read_manifest(build_dir: Path, prefix: str) -> list[Path]:
    candidates = sorted(build_dir.glob(f"{prefix}-*.txt"))
    if not candidates:
        raise SystemExit(
            f"找不到 {prefix}-*.txt；请确认 CMake 以 -DYUN_BUILD_SDK=ON 配置"
        )
    release = [path for path in candidates if path.name.endswith("-Release.txt")]
    manifest = release[0] if release else candidates[0]

    paths: list[Path] = []
    seen: set[Path] = set()
    for line in manifest.read_text(encoding="utf-8").splitlines():
        text = line.strip()
        if not text:
            continue
        path = Path(text)
        if not path.is_absolute():
            path = (build_dir / path).resolve()
        if path in seen:
            continue
        seen.add(path)
        paths.append(path)
    if not paths:
        raise SystemExit(f"清单为空: {manifest}")
    return paths


def _merge_archives(archives: list[Path], target: Path, tag: str) -> None:
    target.parent.mkdir(parents=True, exist_ok=True)
    if target.exists():
        target.unlink()
    for archive in archives:
        if not archive.is_file():
            raise SystemExit(f"静态库不存在: {archive}")

    if tag.startswith("macos-"):
        _run(["libtool", "-static", "-o", str(target), *map(str, archives)])
    elif tag.startswith("linux-"):
        if any(" " in str(path) for path in (*archives, target)):
            raise SystemExit(
                "GNU ar 的 MRI 脚本不支持带空格的路径；"
                "请把工作区或 Conan 缓存放到无空格目录"
            )
        script = "\n".join(
            [
                f"CREATE {target}",
                *(f"ADDLIB {archive}" for archive in archives),
                "SAVE",
                "END",
                "",
            ]
        )
        print(f"+ ar -M ({len(archives)} archives)", flush=True)
        subprocess.run(["ar", "-M"], input=script, text=True, check=True)
    elif tag.startswith("windows-"):
        archiver = shutil.which("llvm-lib") or shutil.which("lib")
        if archiver is None:
            raise SystemExit(
                "找不到 llvm-lib 或 lib；请安装 LLVM，或从 VS 开发者环境运行"
            )
        _run(
            [
                archiver,
                "/nologo",
                f"/OUT:{target}",
                *map(str, archives),
            ]
        )
    else:
        raise SystemExit(f"未知平台标签: {tag}")

    if not target.is_file() or target.stat().st_size == 0:
        raise SystemExit(f"归档生成失败: {target}")
    print(f"合并静态库: {target} ({target.stat().st_size} bytes)", flush=True)


def _component_for(include: Path) -> str | None:
    try:
        relative = include.relative_to(YUN_ROOT)
    except ValueError:
        return None
    if len(relative.parts) == 2 and relative.parts[1] == "include":
        component = relative.parts[0]
        if component in COMPONENTS:
            return component
    return None


def _stage_sdk(
    sdk_root: Path,
    version: str,
    tag: str,
    archives: list[Path],
    includes: list[Path],
) -> None:
    if sdk_root.exists():
        shutil.rmtree(sdk_root)

    include_root = sdk_root / "include"
    include_root.mkdir(parents=True)
    (sdk_root / "lib" / "cmake" / "yun").mkdir(parents=True)
    (sdk_root / "src").mkdir()

    copied: set[Path] = set()
    for include in sorted(includes):
        include = include.resolve()
        if include in copied or not include.is_dir():
            continue
        copied.add(include)
        component = _component_for(include)
        if component is not None:
            destination = include_root / "yun" / component
        else:
            destination = include_root
        shutil.copytree(include, destination, dirs_exist_ok=True)

    for component in COMPONENTS:
        if not (include_root / "yun" / component).is_dir():
            raise SystemExit(f"SDK 缺少组件头文件目录: include/yun/{component}")

    lib_name = _lib_name(tag)
    _merge_archives(archives, sdk_root / "lib" / lib_name, tag)

    config = SDK_CONFIG_TEMPLATE.read_text(encoding="utf-8")
    config = config.replace("@YUN_SDK_LIB_NAME@", lib_name)
    config = config.replace("@YUN_SDK_SYSTEM_LIBS@", _system_libs(tag))
    config = config.replace("@YUN_VERSION@", version)
    _write_text(sdk_root / "lib" / "cmake" / "yun" / "yunConfig.cmake", config)

    version_file = SDK_VERSION_TEMPLATE.read_text(encoding="utf-8")
    version_file = version_file.replace("@YUN_VERSION@", version)
    _write_text(
        sdk_root / "lib" / "cmake" / "yun" / "yunConfigVersion.cmake", version_file
    )

    shutil.copy2(BINDING_SOURCE, sdk_root / "src" / "yun_solver_binding.cpp")
    shutil.copy2(SDK_CMAKE, sdk_root / "CMakeLists.txt")
    shutil.copy2(LICENSE_FILE, sdk_root / "LICENSE")
    _write_text(sdk_root / "VERSION", f"{version}\n")
    _write_text(sdk_root / "SDK_PLATFORM", f"{tag}\n")
    print(f"SDK staged: {sdk_root}", flush=True)


def _make_tar(sdk_root: Path, out_dir: Path, version: str, tag: str) -> Path:
    out_dir.mkdir(parents=True, exist_ok=True)
    name = f"yun-sdk-{version}-{tag}.tar.gz"
    target = out_dir / name
    with tarfile.open(target, "w:gz") as archive:
        for path in sorted(sdk_root.rglob("*")):
            archive.add(
                path,
                arcname=path.relative_to(sdk_root).as_posix(),
                recursive=False,
            )
    digest = hashlib.sha256(target.read_bytes()).hexdigest()
    _write_text(out_dir / f"{name}.sha256", f"{digest}  {name}\n")
    print(f"SDK tarball: {target}", flush=True)
    print(f"sha256: {digest}", flush=True)
    return target


def main() -> None:
    parser = argparse.ArgumentParser(description="构建 yun 预编译 SDK")
    parser.add_argument(
        "--version", default=None, help="SDK 版本（默认取 CMake project 版本）"
    )
    parser.add_argument("--out-dir", type=Path, default=DEFAULT_OUT_DIR)
    parser.add_argument("--build-dir", type=Path, default=DEFAULT_BUILD_DIR)
    parser.add_argument("--deployment-target", default="14.0")
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 1)
    parser.add_argument("--clean", action="store_true")
    args = parser.parse_args()

    tag = _platform_tag()
    version = args.version or _project_version()

    cmake = _require_tool("cmake", "brew install cmake / apt install cmake")
    conan = _require_tool("conan", "python -m pip install 'conan>=2.33,<3'")
    if tag.startswith("macos-"):
        _require_tool("libtool", "libtool 应随 Xcode Command Line Tools 提供")
    elif tag.startswith("linux-"):
        _require_tool("ar", "binutils 应随 GCC 提供")
    elif tag.startswith("windows-"):
        _require_tool("clang-cl", "安装 LLVM 或从 VS 开发者环境运行")
        _require_tool("ninja", "python -m pip install ninja")
        if shutil.which("llvm-lib") is None and shutil.which("lib") is None:
            raise SystemExit(
                "找不到 llvm-lib 或 lib；请安装 LLVM，或从 VS 开发者环境运行"
            )
    if not CONAN_LOCK.is_file():
        raise SystemExit(f"找不到依赖锁文件: {CONAN_LOCK}")

    _ensure_conan_profile(conan)

    build_dir = args.build_dir.expanduser().resolve()
    if args.clean and build_dir.exists():
        print(f"清理构建目录 {build_dir}", flush=True)
        shutil.rmtree(build_dir)

    print(f"SDK version: {version} ({tag})", flush=True)

    _run(_configure_args(cmake, build_dir, tag, args.deployment_target))
    _run(
        [
            cmake,
            "--build",
            str(build_dir),
            "--target",
            BUILD_TARGET,
            "--parallel",
            str(args.jobs),
        ]
    )

    archives = _read_manifest(build_dir, "sdk-archives")
    includes = _read_manifest(build_dir, "sdk-includes")

    sdk_root = build_dir / f"yun-sdk-{version}-{tag}"
    _stage_sdk(sdk_root, version, tag, archives, includes)
    _make_tar(sdk_root, args.out_dir.expanduser().resolve(), version, tag)


if __name__ == "__main__":
    main()
