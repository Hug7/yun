
set(ROUTE_PLAN_DOMAIN_DIR "${PROJECT_SOURCE_DIR}/domain/")
file(GLOB ROUTE_PLAN_DOMAIN_INCLUDE_HEADERS
        "${ROUTE_PLAN_DOMAIN_DIR}/include/*.h"
)
file(GLOB ROUTE_PLAN_DOMAIN_SOURCE_FILES
        "${ROUTE_PLAN_DOMAIN_DIR}/*.cpp"
)

if (NOT ROUTE_PLAN_DOMAIN_SOURCE_FILES)
    message(WARNING "No domain source found")
    file(WRITE "${ROUTE_PLAN_DOMAIN_DIR}/domain.cpp" " ")
    file(GLOB ROUTE_PLAN_DOMAIN_SOURCE_FILES
        "${ROUTE_PLAN_DOMAIN_DIR}/*.cpp"
    )
endif ()

if (NOT ROUTE_PLAN_DOMAIN_INCLUDE_HEADERS)
    message(FATAL_ERROR "No domain headers found")
elseif (NOT ROUTE_PLAN_DOMAIN_SOURCE_FILES)
    message(FATAL_ERROR "No domain source found")
else ()
    add_library(route_domain STATIC
            ${ROUTE_PLAN_DOMAIN_INCLUDE_HEADERS}
            ${ROUTE_PLAN_DOMAIN_SOURCE_FILES}
    )
endif ()

target_include_directories(route_domain
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include/wx_route/domain>
)

target_link_libraries(route_domain PUBLIC route_common)

# 安装静态库和头文件
install(TARGETS route_domain
    EXPORT wxConfig
    # EXPORT wxTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER INCLUDES DESTINATION include
)

