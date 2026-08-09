# 1. file collection
set(YUN_COMMON_DIR "${PROJECT_SOURCE_DIR}/common/")
file(GLOB YUN_COMMON_SOURCE_FILES CONFIGURE_DEPENDS
        "${YUN_COMMON_DIR}/include/*.h"
)

# 2. library creation
add_library(yun_common INTERFACE ${YUN_COMMON_SOURCE_FILES})

# 3. set library properties
set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR} ${CMAKE_PREFIX_PATH})

find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(yaml-cpp REQUIRED)
target_link_libraries(yun_common INTERFACE fmt::fmt spdlog::spdlog yaml-cpp::yaml-cpp)

# 4. include directories
target_include_directories(yun_common
    INTERFACE 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/yun/common>
)

# 5. install
install(TARGETS yun_common
    EXPORT yunConfig
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER INCLUDES DESTINATION include
)

