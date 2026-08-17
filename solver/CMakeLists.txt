# 1. file collection
set(YUN_SOLVER_DIR "${PROJECT_SOURCE_DIR}/solver/")
file(GLOB YUN_SOLVER_INCLUDE_HEADERS CONFIGURE_DEPENDS
        "${YUN_SOLVER_DIR}/include/*.h"
)
file(GLOB YUN_SOLVER_SOURCE_FILES CONFIGURE_DEPENDS
        "${YUN_SOLVER_DIR}/src/*.cpp"
)

# 2. library creation
add_library(yun_solver STATIC)
target_sources(yun_solver
    PRIVATE ${YUN_SOLVER_SOURCE_FILES}
    PUBLIC FILE_SET public_solver_headers TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
        FILES ${YUN_SOLVER_INCLUDE_HEADERS}
)

# 3. include directories
target_include_directories(yun_solver
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/yun/solver>
)

# 4. linked dependencies
target_link_libraries(yun_solver PUBLIC yun_common yun_domain yun_scenario yun_plan)

# 5. install
install(TARGETS yun_solver
    EXPORT yunConfig
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    FILE_SET public_solver_headers DESTINATION include/yun/solver
)
