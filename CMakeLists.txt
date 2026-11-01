cmake_minimum_required(VERSION 3.24)
cmake_policy(SET CMP0091 NEW)

project(yun VERSION 0.1.0 LANGUAGES C CXX)

export(PACKAGE yun)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── C++ 标准 ──────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── 编译器支持门禁：Linux GCC / macOS AppleClang / Windows clang-cl ──
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
    message(FATAL_ERROR
        "yun 支持 Linux GCC、macOS AppleClang、Windows clang-cl；"
        "当前 C++ 编译器为 ${CMAKE_CXX_COMPILER_ID}，不支持 MSVC。")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
   AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13.2)
    message(FATAL_ERROR "需要 GCC >= 13.2（当前 ${CMAKE_CXX_COMPILER_VERSION}）")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
   AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 17.0)
    message(FATAL_ERROR "需要 Clang >= 17（当前 ${CMAKE_CXX_COMPILER_VERSION}）")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"
   AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16.0)
    message(FATAL_ERROR "需要 AppleClang >= 16（当前 ${CMAKE_CXX_COMPILER_VERSION}）")
endif()

# Linux 只支持 GCC；Clang 仅用于 macOS（AppleClang）和 Windows（clang-cl）
if(UNIX AND NOT APPLE AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "Linux 仅支持 GCC（当前 ${CMAKE_CXX_COMPILER_ID}）")
endif()

if(WIN32)
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
       OR (DEFINED CMAKE_CXX_COMPILER_FRONTEND_VARIANT
           AND NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
        message(FATAL_ERROR
            "Windows 仅支持 clang-cl（当前 ${CMAKE_CXX_COMPILER_ID}/"
            "${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}）")
    endif()
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "Windows 仅支持 64 位构建")
    endif()
    if(MSVC AND DEFINED CMAKE_MSVC_RUNTIME_LIBRARY
       AND NOT CMAKE_MSVC_RUNTIME_LIBRARY MATCHES "DLL")
        message(FATAL_ERROR "Windows 仅支持动态 MSVC 运行时（/MD 或 /MDd）")
    endif()
endif()

# ── 构建类型默认 Debug ────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# ── 编译器诊断标志 ────────────────────────────────────────
if(MSVC)
    # clang-cl 也定义 MSVC；真正的 MSVC 已在上面被拒绝
    add_compile_options(/W4 /utf-8)
    add_compile_options($<$<CONFIG:Release>:/O2>)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    add_compile_options($<$<CONFIG:Release>:-O2>)
endif()

# ── 平台特定设置 ──────────────────────────────────────────
if(APPLE)
    # macOS：可选指定架构，默认跟随主机
    # set(CMAKE_OSX_ARCHITECTURES "arm64")   # M 系列
    # set(CMAKE_OSX_ARCHITECTURES "x86_64")  # Intel
elseif(WIN32)
    # Windows：MSVC 需要 _CRT_SECURE_NO_WARNINGS 等，视需要开启
    # add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
elseif(UNIX)
    # Linux
endif()

# ── ASan（按需开启）──────────────────────────────────────
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
if(ENABLE_ASAN)
    if(WIN32)
        message(FATAL_ERROR "ENABLE_ASAN 暂不支持 Windows；本轮 ASan 只在 macOS/Linux 启用")
    endif()
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

# add subdirectories
add_subdirectory(common)
add_subdirectory(scenario)
add_subdirectory(problem)
add_subdirectory(algorithm)
add_subdirectory(solver)
option(YUN_BUILD_PYTHON "Build the Python pybind11 binding" OFF)
if(YUN_BUILD_PYTHON)
    add_subdirectory(python)
endif()
option(YUN_BUILD_TESTS "Build the C++ unit tests" ON)
if(YUN_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()


# 总安装配置
install(DIRECTORY common/include/ DESTINATION include/yun/common)

# 生成 CMake 配置文件
install(
    EXPORT yunConfig
    FILE yunConfig.cmake
    DESTINATION lib/cmake/yun
    NAMESPACE yun::
)
