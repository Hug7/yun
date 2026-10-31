cmake_minimum_required(VERSION 3.24)
project(yun_python_binding LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(YUN_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
if(NOT EXISTS "${YUN_SDK_ROOT}/lib/libyun_sdk.a")
    message(FATAL_ERROR "SDK 缺少 lib/libyun_sdk.a，请检查 yun-sdk 包是否完整")
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)
find_package(pybind11 CONFIG REQUIRED)

add_library(yun_sdk STATIC IMPORTED)
set_target_properties(yun_sdk PROPERTIES
    IMPORTED_LOCATION "${YUN_SDK_ROOT}/lib/libyun_sdk.a"
)
target_include_directories(yun_sdk INTERFACE
    "${YUN_SDK_ROOT}/include"
    "${YUN_SDK_ROOT}/include/yun/common"
    "${YUN_SDK_ROOT}/include/yun/scenario"
    "${YUN_SDK_ROOT}/include/yun/problem"
    "${YUN_SDK_ROOT}/include/yun/algorithm"
    "${YUN_SDK_ROOT}/include/yun/solver"
)

pybind11_add_module(yun_python_binding WITH_SOABI "${YUN_SDK_ROOT}/src/yun_solver_binding.cpp")
target_compile_definitions(yun_python_binding PRIVATE
    SPDLOG_COMPILED_LIB
    SPDLOG_FMT_EXTERNAL
    YAML_CPP_STATIC_DEFINE
)
target_link_libraries(yun_python_binding PRIVATE yun_sdk)
set_target_properties(yun_python_binding PROPERTIES
    OUTPUT_NAME "_core"
    INSTALL_RPATH ""
)
