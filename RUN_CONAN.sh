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
    echo "  $0 --mode debug    Install dependencies for debug mode"
    echo "  $0 --mode release  Install dependencies for release mode"
    echo "  $0 --mode debug --build-dir my_build  Install dependencies in my_build directory"
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

# 检查Conan版本
echo "Checking Conan version..."
conan --version

# 函数：安装 conan 依赖
install_conan_deps() {
    local build_type=$1
    local build_dir="${BUILD_DIR}/${build_type}"
    
    echo "\nInstalling dependencies for ${build_type} mode..."
    
    # 检查是否需要执行 conan install
    CONAN_TOOLCHAIN="${build_dir}/conan_toolchain.cmake"
    
    # 如果 conan_toolchain.cmake 不存在，需要执行 conan install
    if [ ! -f "${CONAN_TOOLCHAIN}" ]; then
        echo "conan_toolchain.cmake not found, running conan install..."
        conan install . --output-folder="${build_dir}" --build=missing -s build_type=${build_type}
        echo "Conan dependencies installed successfully for ${build_type} mode"
    else
        echo "conan_toolchain.cmake exists, skipping conan install for ${build_type} mode"
    fi
}

# 根据选择的模式安装依赖
case "$BUILD_MODE" in
    debug)
        install_conan_deps "Debug"
        echo "\nConan dependencies installation completed!"
        echo "Debug conan_toolchain: ${BUILD_DIR}/Debug/conan_toolchain.cmake"
        ;;
    release)
        install_conan_deps "Release"
        echo "\nConan dependencies installation completed!"
        echo "Release conan_toolchain: ${BUILD_DIR}/Release/conan_toolchain.cmake"
        ;;
    *)
        echo "Invalid build mode: $BUILD_MODE"
        show_help
        exit 1
        ;;
esac
