# 1. file collection
set(YUN_PLAN_DIR "${PROJECT_SOURCE_DIR}/plan/")
file(GLOB YUN_PLAN_INCLUDE_HEADERS
        "${YUN_PLAN_DIR}/include/*.h"
)
file(GLOB YUN_PLAN_SOURCE_FILES
        "${YUN_PLAN_DIR}/src/*.cpp"
)

# 2. library creation
add_library(yun_plan STATIC)
target_sources(yun_plan
    PRIVATE ${YUN_PLAN_SOURCE_FILES}
    PUBLIC FILE_SET public_plan_headers TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
        FILES ${YUN_PLAN_INCLUDE_HEADERS}
)

# 3. linked dependencies
target_link_libraries(yun_plan PUBLIC yun_common yun_domain yun_scenario)

# 4. include directories
target_include_directories(yun_plan
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/yun/plan>
)

# 5. install
install(TARGETS yun_plan
    EXPORT yunConfig
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    FILE_SET public_plan_headers DESTINATION include/yun/plan
)
