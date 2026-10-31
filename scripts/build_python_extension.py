#!/usr/bin/env python3
"""构建 yun 的 Python 扩展（pybind11），供 yunpyopt 打包使用。

只支持 macOS。示例：

    python3.11 scripts/build_python_extension.py
    python3.11 scripts/build_python_extension.py --out ../yunpyopt/src/yunpyopt
    python3.11 scripts/build_python_extension.py --python /path/to/python3.11 --clean
"""

from __future__ import annotations

import argparse
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path

YUN_ROOT = Path(__file__).resolve().parents[1]
CONAN_LOCK = YUN_ROOT / "conan.lock"
DEFAULT_BUILD_DIR = YUN_ROOT / "build" / "python-extension"
TARGET_NAME = "yun_python_binding"


def _run(cmd: list[str], *, cwd: Path | None = None) -> None:
    print("+", " ".join(str(part) for part in cmd), flush=True)
    subprocess.run(cmd, cwd=cwd, check=True)


def _python_info(python: Path) -> tuple[int, int, str]:
    code = (
        "import sys, sysconfig;"
        "print(sys.version_info.major);"
        "print(sys.version_info.minor);"
        "print(sysconfig.get_platform())"
    )
    output = subprocess.check_output([str(python), "-c", code], text=True).splitlines()
    return int(output[0]), int(output[1]), output[2]


def _require_tool(name: str) -> str:
    path = shutil.which(name)
    if path is None:
        raise SystemExit(
            f"找不到 {name}。请先安装：\n"
            "  cmake: brew install cmake\n"
            "  conan: python -m pip install 'conan>=2.33,<3'\n"
        )
    return path


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


def _write_conan_host_profile(build_dir: Path) -> Path:
    """生成 C++20 的 host profile，不修改用户的 default profile。"""
    build_dir.mkdir(parents=True, exist_ok=True)
    profile = build_dir / "conan-host-profile"
    profile.write_text("include(default)\n[settings]\ncompiler.cppstd=20\n")
    return profile


def _build(
    python: Path,
    build_dir: Path,
    deployment_target: str,
    jobs: int,
    clean: bool,
) -> Path:
    if not CONAN_LOCK.is_file():
        raise SystemExit(f"找不到依赖锁文件: {CONAN_LOCK}")

    _require_tool("cmake")
    conan = _require_tool("conan")
    _ensure_conan_profile(conan)

    major, minor, platform_tag = _python_info(python)
    if (major, minor) < (3, 11):
        raise SystemExit(f"需要 Python >= 3.11，当前为 {major}.{minor}")

    print(f"目标 Python: {python} ({major}.{minor}, {platform_tag})", flush=True)

    if clean and build_dir.exists():
        print(f"清理构建目录 {build_dir}", flush=True)
        shutil.rmtree(build_dir)

    host_profile = _write_conan_host_profile(build_dir)
    _run(
        [
            "cmake",
            "-S",
            str(YUN_ROOT),
            "-B",
            str(build_dir),
            "-DCMAKE_BUILD_TYPE=Release",
            "-DYUN_BUILD_PYTHON=ON",
            "-DYUN_BUILD_TESTS=OFF",
            f"-DCMAKE_PROJECT_TOP_LEVEL_INCLUDES={YUN_ROOT / 'conan_provider.cmake'}",
            f"-DCONAN_HOST_PROFILE={host_profile}",
            f"-DCONAN_INSTALL_ARGS=--build=missing;--lockfile={CONAN_LOCK}",
            f"-DPython3_EXECUTABLE={python}",
            "-DCMAKE_SKIP_RPATH=ON",
            "-DCMAKE_OSX_ARCHITECTURES=arm64",
            f"-DCMAKE_OSX_DEPLOYMENT_TARGET={deployment_target}",
        ]
    )
    _run(
        [
            "cmake",
            "--build",
            str(build_dir),
            "--target",
            TARGET_NAME,
            "-j",
            str(jobs),
        ]
    )

    modules = sorted((build_dir / "python").glob("_core*.so"))
    if not modules:
        raise SystemExit(f"未找到扩展模块：{build_dir / 'python'}/_core*.so")
    if len(modules) > 1:
        names = ", ".join(module.name for module in modules)
        raise SystemExit(f"发现多个扩展模块（{names}），请加 --clean 重建")

    module = modules[0]
    print(f"扩展模块: {module}", flush=True)
    return module


def _smoke_test(python: Path, module: Path) -> None:
    code = (
        "import importlib.util;"
        f"path = {str(module)!r};"
        "spec = importlib.util.spec_from_file_location('_core', path);"
        "mod = importlib.util.module_from_spec(spec);"
        "spec.loader.exec_module(mod);"
        "print('RESULT_KEYS', mod.RESULT_KEYS)"
    )
    _run([str(python), "-c", code])


def _copy_out(module: Path, out_dir: Path) -> Path:
    out_dir.mkdir(parents=True, exist_ok=True)
    for stale in out_dir.glob("_core.*.so"):
        stale.unlink()
    target = out_dir / module.name
    shutil.copy2(module, target)
    print(f"已复制到 {target}", flush=True)
    return target


def main() -> None:
    if platform.system() != "Darwin":
        raise SystemExit("当前只支持 macOS")

    parser = argparse.ArgumentParser(description="构建 yun 的 Python 扩展")
    parser.add_argument(
        "--python",
        default=sys.executable,
        help="目标 Python 解释器（默认当前解释器）",
    )
    parser.add_argument("--out", type=Path, help="构建后把 _core*.so 复制到该目录")
    parser.add_argument("--build-dir", type=Path, default=DEFAULT_BUILD_DIR)
    parser.add_argument(
        "--deployment-target",
        default=os.environ.get("MACOSX_DEPLOYMENT_TARGET", "14.0"),
        help="macOS deployment target（默认 14.0）",
    )
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 1)
    parser.add_argument("--clean", action="store_true", help="删除构建目录后重建")
    args = parser.parse_args()

    python = Path(args.python).expanduser()
    if not python.is_file():
        resolved = shutil.which(args.python)
        if not resolved:
            raise SystemExit(f"找不到 Python：{args.python}")
        python = Path(resolved)
    python = python.resolve()

    module = _build(
        python,
        args.build_dir.expanduser().resolve(),
        args.deployment_target,
        args.jobs,
        args.clean,
    )
    _smoke_test(python, module)
    if args.out:
        _copy_out(module, args.out.expanduser().resolve())


if __name__ == "__main__":
    main()
