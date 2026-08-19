# 1. file collection
set(YUN_PROBLEM_DIR "${PROJECT_SOURCE_DIR}/problem/")
file(GLOB YUN_PROBLEM_INCLUDE_HEADERS
        "${YUN_PROBLEM_DIR}/include/*.h"
)
file(GLOB YUN_PROBLEM_SOURCE_FILES
        "${YUN_PROBLEM_DIR}/src/*.cpp"
)

# 2. library creation
add_library(yun_problem STATIC)
target_sources(yun_problem
    PRIVATE ${YUN_PROBLEM_SOURCE_FILES}
    PUBLIC FILE_SET public_problem_headers TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
        FILES ${YUN_PROBLEM_INCLUDE_HEADERS}
)

# 3. linked dependencies
target_link_libraries(yun_problem PUBLIC yun_common yun_domain yun_scenario)

# 4. include directories
target_include_directories(yun_problem
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/yun/problem>
)

# 5. install
install(TARGETS yun_problem
    EXPORT yunConfig
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    FILE_SET public_problem_headers DESTINATION include/yun/problem
)
