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
    echo "  $0 --mode debug    Install dependencies and build debug mode"
    echo "  $0 --mode release  Install dependencies and build release mode"
    echo "  $0 --mode debug --build-dir my_build  Use my_build as build directory"
    echo ""
    echo "Note: This script will run RUN_CONAN.sh and RUN_BUILD.sh in sequence."
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

# 处理构建模式
case "$BUILD_MODE" in
    debug|release)
        # 先运行 conan 安装依赖
        echo "Running RUN_CONAN.sh..."
        ./RUN_CONAN.sh --mode "$BUILD_MODE" --build-dir "$BUILD_DIR"
        
        # 检查 conan 安装是否成功
        if [ $? -ne 0 ]; then
            echo "Error: RUN_CONAN.sh failed"
            exit 1
        fi
        
        # 再运行构建
        echo "Running RUN_BUILD.sh..."
        ./RUN_BUILD.sh --mode "$BUILD_MODE" --build-dir "$BUILD_DIR"
        
        # 检查构建是否成功
        if [ $? -ne 0 ]; then
            echo "Error: RUN_BUILD.sh failed"
            exit 1
        fi
        
        echo "All operations completed successfully!"
        echo "Build directory: $BUILD_DIR"
        ;;
    *)
        echo "Invalid build mode: $BUILD_MODE"
        show_help
        exit 1
        ;;
esac
