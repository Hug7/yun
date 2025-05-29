set(ROUTE_PLAN_COMMON_DIR "${PROJECT_SOURCE_DIR}/common/")
file(GLOB ROUTE_PLAN_COMMON_SOURCE_FILES
        "${ROUTE_PLAN_COMMON_DIR}/include/*.h"
)

add_library(route_common INTERFACE ${ROUTE_PLAN_COMMON_SOURCE_FILES})

# target_include_directories(route_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

find_package(fmt)
find_package(spdlog)
find_package(yaml-cpp)
target_link_libraries(route_common INTERFACE fmt::fmt spdlog::spdlog yaml-cpp::yaml-cpp)
# target_link_libraries(route_common INTERFACE ${CONAN_LIBS})

# find_path(RAPIDCSV_INCLUDE_DIRS "rapidcsv.h")
# target_include_directories(route_common PUBLIC ${RAPIDCSV_INCLUDE_DIRS})
# target_link_libraries(route_common PUBLIC rapidcsv)

target_include_directories(route_common
    INTERFACE 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include/wx_route/common>
)

# 安装静态库和头文件
install(TARGETS route_common
    # EXPORT wxTargets
    EXPORT wxConfig
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER INCLUDES DESTINATION include
)

