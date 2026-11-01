cmake_minimum_required(VERSION 3.24)
project(yun_python_binding LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# SDK 自带 lib/cmake/yun，优先用自身；也支持消费方通过 CMAKE_PREFIX_PATH 查找。
find_package(yun CONFIG REQUIRED HINTS "${CMAKE_CURRENT_SOURCE_DIR}")
find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)
find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(yun_python_binding WITH_SOABI
    "${CMAKE_CURRENT_SOURCE_DIR}/src/yun_solver_binding.cpp")
target_link_libraries(yun_python_binding PRIVATE yun::sdk)
set_target_properties(yun_python_binding PROPERTIES
    OUTPUT_NAME "_core"
    INSTALL_RPATH ""
)
