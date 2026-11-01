#!/usr/bin/env python3
"""Run one yun functional case on macOS/Linux/Windows.

Usage:
    python scripts/ci/run_functional_case.py \
        --build-dir build/ci \
        --case-zip .ci/yun-cases/feature_tests/C_SUC_001.zip \
        --work-dir "$RUNNER_TEMP/yun-case"
"""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import zipfile
from pathlib import Path

REQUIRED_OUTPUTS = (
    "PlanSummary.json",
    "Loads.json",
    "LoadLocations.json",
    "UnassignedCargoOrders.json",
)


def _find_executable(build_dir: Path) -> Path:
    stem = build_dir / "tests" / "vrp_exec"
    for candidate in (stem, stem.with_suffix(".exe")):
        if candidate.is_file():
            return candidate
    raise SystemExit(f"找不到 vrp_exec（尝试过 {stem} 和 {stem}.exe）")


def _extract_case(case_zip: Path, work_dir: Path) -> Path:
    case_root = work_dir / "case"
    if case_root.exists():
        shutil.rmtree(case_root)
    case_root.mkdir(parents=True)
    with zipfile.ZipFile(case_zip) as archive:
        archive.extractall(case_root)
    candidate = case_root / "C_SUC_001"
    return candidate if candidate.is_dir() else case_root


def _validate(case_dir: Path) -> tuple[Path, dict]:
    output_dir = case_dir / "output"
    if not output_dir.is_dir():
        raise SystemExit(f"缺少输出目录: {output_dir}")
    outputs = sorted(path for path in output_dir.iterdir() if path.is_dir())
    if not outputs:
        raise SystemExit(f"{output_dir} 下没有输出目录")
    latest = outputs[-1]
    for name in REQUIRED_OUTPUTS:
        path = latest / name
        if not path.is_file():
            raise SystemExit(f"缺少输出文件: {path}")
        json.loads(path.read_text(encoding="utf-8"))
    summary = json.loads((latest / "PlanSummary.json").read_text(encoding="utf-8"))
    if summary["TotalCargoOrderCount"] <= 0:
        raise SystemExit(f"TotalCargoOrderCount 非法: {summary}")
    if summary["TotalLoadCount"] <= 0:
        raise SystemExit(f"TotalLoadCount 非法: {summary}")
    if summary["UnassignedCargoOrderCount"] != 0:
        raise SystemExit(f"存在未指派订单: {summary}")
    return latest, summary


def main() -> None:
    parser = argparse.ArgumentParser(description="运行一个 yun 功能测试用例")
    parser.add_argument("--build-dir", type=Path, default=Path("build/ci"))
    parser.add_argument("--case-zip", type=Path, required=True)
    parser.add_argument("--work-dir", type=Path, default=None)
    args = parser.parse_args()

    work_dir = args.work_dir or Path(os.environ.get("RUNNER_TEMP", ".")) / "yun-case"
    work_dir = work_dir.resolve()
    work_dir.mkdir(parents=True, exist_ok=True)

    case_dir = _extract_case(args.case_zip.resolve(), work_dir)
    log_dir = work_dir / "log"
    if log_dir.exists():
        shutil.rmtree(log_dir)
    log_dir.mkdir(parents=True)

    exe = _find_executable(args.build_dir.resolve())
    subprocess.run(
        [str(exe), str(case_dir), str(log_dir), "debug"], check=True, cwd=work_dir
    )

    latest, summary = _validate(case_dir)
    print(
        "C_SUC_001 OK:",
        f"loads={summary['TotalLoadCount']}",
        f"objective={summary['TotalObjective']}",
        f"output={latest}",
    )


if __name__ == "__main__":
    main()
