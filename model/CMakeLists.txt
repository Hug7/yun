# 1. file collection
set(YUN_MODEL_DIR "${PROJECT_SOURCE_DIR}/model/")
file(GLOB YUN_MODEL_INCLUDE_HEADERS
        "${YUN_MODEL_DIR}/include/*.h"
)
file(GLOB YUN_MODEL_SOURCE_FILES
        "${YUN_MODEL_DIR}/src/*.cpp"
)

# 2. library creation
add_library(yun_model STATIC)
target_sources(yun_model
    PRIVATE ${YUN_MODEL_SOURCE_FILES}
    PUBLIC FILE_SET public_model_headers TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
        FILES ${YUN_MODEL_INCLUDE_HEADERS}
)

# 3. linked dependencies
target_link_libraries(yun_model PUBLIC yun_common yun_domain yun_scenario)

# 4. include directories
target_include_directories(yun_model
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/yun/domain>
)

# 5. install
install(TARGETS yun_model
    EXPORT yunConfig
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    FILE_SET public_model_headers DESTINATION include/yun/model
)

