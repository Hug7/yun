# 1. file collection
set(YUN_ALGORITHM_DIR "${PROJECT_SOURCE_DIR}/algorithm/")
file(GLOB YUN_ALGORITHM_INCLUDE_HEADERS CONFIGURE_DEPENDS
        "${YUN_ALGORITHM_DIR}/include/*.h"
)
file(GLOB YUN_ALGORITHM_SOURCE_FILES CONFIGURE_DEPENDS
        "${YUN_ALGORITHM_DIR}/src/*.cpp"
)

# 2. library creation
add_library(yun_algorithm STATIC)
target_sources(yun_algorithm
        PRIVATE ${YUN_ALGORITHM_SOURCE_FILES}
        PUBLIC FILE_SET public_algorithm_headers TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
        FILES ${YUN_ALGORITHM_INCLUDE_HEADERS}
)

# 3. include directories
target_include_directories(yun_algorithm
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/yun/algorithm>
)

# 4. linked dependencies
target_link_libraries(yun_algorithm PUBLIC yun_common)

# 5. install
install(TARGETS yun_algorithm
        EXPORT yunConfig
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        FILE_SET public_algorithm_headers DESTINATION include/yun/algorithm
)
