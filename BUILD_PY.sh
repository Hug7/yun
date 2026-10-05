#!/usr/bin/env bash
set -euo pipefail

PYTHON_BIN="${PYTHON_BIN:-python3.11}"
YUN_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${BUILD_PY_BUILD_DIR:-${YUN_ROOT}/cmake-build-python}"
YUNPY_SRC="${YUNPY_SRC:-${YUN_ROOT}/../yunpy/src/yunpy}"

python_bin="$(command -v "$PYTHON_BIN")"
cmake -S "$YUN_ROOT" -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE=Release \
  -DYUN_BUILD_PYTHON=ON \
  -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=conan_provider.cmake \
  -DCONAN_HOST_PROFILE=default \
  -DPython3_EXECUTABLE="$python_bin"
cmake --build "$BUILD_DIR" --target yun_python_binding -j "$(sysctl -n hw.ncpu 2>/dev/null || nproc)"

module="$(find "$BUILD_DIR/python" -maxdepth 1 -name '_core.*.so' -print -quit)"
if [[ -z "$module" ]]; then
  echo "未找到 Python 扩展模块" >&2
  exit 1
fi

mkdir -p "$YUNPY_SRC"
cp "$module" "$YUNPY_SRC/"
