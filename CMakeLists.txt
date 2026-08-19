cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0091 NEW)

# include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake)

project(yun VERSION 0.1.0 LANGUAGES C CXX)

export(PACKAGE yun)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── C++ 标准 ──────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 包含 Conan 生成的文件
if(EXISTS ${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
endif()

# ── 构建类型默认 Debug ────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# ── 编译器诊断标志（GCC / Clang 通用）──────────────────────
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
    )
    # Release 下开启优化
    add_compile_options($<$<CONFIG:Release>:-O2>)
elseif(MSVC)
    add_compile_options(/W4 /utf-8)
    add_compile_options($<$<CONFIG:Release>:/O2>)
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
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    elseif(MSVC)
        add_compile_options(/fsanitize=address)
    endif()
endif()

# add subdirectories
add_subdirectory(common)
add_subdirectory(domain)
add_subdirectory(scenario)
add_subdirectory(problem)
add_subdirectory(algorithm)
add_subdirectory(solver)
add_subdirectory(tests)


# 总安装配置
install(DIRECTORY common/include/ DESTINATION include/yun/common)
install(DIRECTORY domain/include/ DESTINATION include/yun/domain)
# install(DIRECTORY core/tsp/include/ DESTINATION include/yun/tsp)

# 生成 CMake 配置文件
install(
    EXPORT yunConfig
    FILE yunConfig.cmake
    DESTINATION lib/cmake/yun
    NAMESPACE yun::
)

# include(CMakePackageConfigHelpers)
# configure_package_config_file(
#     ${CMAKE_CURRENT_SOURCE_DIR}/yunConfig.cmake.in
#     ${CMAKE_CURRENT_BINARY_DIR}/yunConfig.cmake
#     INSTALL_DESTINATION lib/cmake/yun
# )

# write_basic_package_version_file(
#     ${CMAKE_CURRENT_BINARY_DIR}/yunConfigVersion.cmake
#     VERSION 1.0.0
#     COMPATIBILITY AnyNewerVersion
# )

# install(
#     EXPORT yunTargets
#     FILE yunTargets.cmake
#     NAMESPACE yun::
#     DESTINATION lib/cmake/yun
# )

# install(
#     FILES
#     ${CMAKE_CURRENT_BINARY_DIR}/yunConfig.cmake
#     ${CMAKE_CURRENT_BINARY_DIR}/yunConfigVersion.cmake
#     DESTINATION lib/cmake/yun
# )