# 1. file collection
set(YUN_DOMAIN_DIR "${PROJECT_SOURCE_DIR}/domain/")
file(GLOB YUN_DOMAIN_INCLUDE_HEADERS
        "${YUN_DOMAIN_DIR}/include/*.h"
)
file(GLOB YUN_DOMAIN_SOURCE_FILES
        "${YUN_DOMAIN_DIR}/src/*.cpp"
)

# 2. library creation
add_library(yun_domain STATIC)
target_sources(yun_domain
    PRIVATE ${YUN_DOMAIN_SOURCE_FILES}
    PUBLIC FILE_SET public_domain_headers TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
        FILES ${YUN_DOMAIN_INCLUDE_HEADERS}
)

# 3. linked dependencies
target_link_libraries(yun_domain PUBLIC yun_common)

# 4. include directories
target_include_directories(yun_domain
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/yun/domain>
)

# 5. install
install(TARGETS yun_domain
    EXPORT yunConfig
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    FILE_SET public_domain_headers DESTINATION include/yun/domain
)

