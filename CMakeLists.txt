cmake_minimum_required(VERSION 3.10.0)
cmake_policy(SET CMP0091 NEW)

include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake)

project(wx_route_plan VERSION 0.1.0 LANGUAGES C CXX)

export(PACKAGE wx_route_plan)

set(CMAKE_C_COMPILER "F:\\Program Files (x86)\\LLVM\\bin\\clang.exe")
set(CMAKE_CXX_COMPILER "F:\\Program Files (x86)\\LLVM\\bin\\clang++.exe")

# set(CMAKE_C_COMPILER "F:\\Program Files (x86)\\LLVM\\bin\\clang")
# set(CMAKE_CXX_COMPILER "F:\\Program Files (x86)\\LLVM\\bin\\clang++")


# # 启用 ASan 编译选项
# add_compile_options(-fsanitize=address)
# # 启用 ASan 链接选项
# add_link_options(-fsanitize=address)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
# set(CMAKE_BUILD_TYPE Release)
set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a")
# set(CMAKE_CXX_EXTENSIONS OFF) # 确保不使用GNU扩展
# set(CMAKE_OSX_ARCHITECTURES "arm64")

# add subdirectories
add_subdirectory(common)
add_subdirectory(domain)
add_subdirectory(core)
add_subdirectory(tests)


# 总安装配置
install(DIRECTORY common/include/ DESTINATION include/wx_route/common)
install(DIRECTORY domain/include/ DESTINATION include/wx_route/domain)
install(DIRECTORY core/tsp/include/ DESTINATION include/wx_route/tsp)

# 生成 CMake 配置文件
install(
    EXPORT wxConfig
    FILE wxConfig.cmake
    DESTINATION lib/cmake/wx_route
    NAMESPACE wx_route::
)

# include(CMakePackageConfigHelpers)
# configure_package_config_file(
#     ${CMAKE_CURRENT_SOURCE_DIR}/wxConfig.cmake.in
#     ${CMAKE_CURRENT_BINARY_DIR}/wxConfig.cmake
#     INSTALL_DESTINATION lib/cmake/wx
# )

# write_basic_package_version_file(
#     ${CMAKE_CURRENT_BINARY_DIR}/wxConfigVersion.cmake
#     VERSION 1.0.0
#     COMPATIBILITY AnyNewerVersion
# )

# install(
#     EXPORT wxTargets
#     FILE wxTargets.cmake
#     NAMESPACE wx::
#     DESTINATION lib/cmake/wx
# )

# install(
#     FILES
#     ${CMAKE_CURRENT_BINARY_DIR}/wxConfig.cmake
#     ${CMAKE_CURRENT_BINARY_DIR}/wxConfigVersion.cmake
#     DESTINATION lib/cmake/wx
# )

# conan install . --output-folder=build --build=missing -s build_type=Debug
