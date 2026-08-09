#!/bin/bash

# 显示帮助信息
show_help() {
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo "  -m, --mode MODE    Build mode (debug, release)"
    echo "  -b, --build-dir DIR  Build directory (default: build)"
    echo "  -h, --help         Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 --mode debug    Build only debug mode"
    echo "  $0 --mode release  Build only release mode"
    echo "  $0 --mode debug --build-dir my_build  Use my_build as build directory"
}

# 默认构建目录
BUILD_DIR="build"

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        -m|--mode)
            BUILD_MODE="$2"
            shift 2
            ;;
        -b|--build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# 检查是否指定了构建模式
if [ -z "$BUILD_MODE" ]; then
    echo "Error: Build mode is required. Please specify --mode option."
    show_help
    exit 1
fi

# 函数：编译项目
build_project() {
    local build_type=$1
    local build_dir="${BUILD_DIR}/${build_type}"
    
    echo "\nBuilding ${build_type} mode..."
    
    # 构建项目
    echo "Building project in ${build_type} mode..."
    cd "${build_dir}"
    cmake ../../ -DCMAKE_BUILD_TYPE=${build_type}
    make -j4
    
    # 回到项目根目录
    cd ../..
}

# 根据选择的模式构建
case "$BUILD_MODE" in
    debug)
        build_project "Debug"
        echo "\nBuild completed successfully!"
        echo "Debug executable: ${BUILD_DIR}/Debug/tests/tsp_exec"
        ;;
    release)
        build_project "Release"
        echo "\nBuild completed successfully!"
        echo "Release executable: ${BUILD_DIR}/Release/tests/tsp_exec"
        ;;
    *)
        echo "Invalid build mode: $BUILD_MODE"
        show_help
        exit 1
        ;;
esac
