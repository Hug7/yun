########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

set(yaml-cpp_COMPONENT_NAMES "")
if(DEFINED yaml-cpp_FIND_DEPENDENCY_NAMES)
  list(APPEND yaml-cpp_FIND_DEPENDENCY_NAMES )
  list(REMOVE_DUPLICATES yaml-cpp_FIND_DEPENDENCY_NAMES)
else()
  set(yaml-cpp_FIND_DEPENDENCY_NAMES )
endif()

########### VARIABLES #######################################################################
#############################################################################################
set(yaml-cpp_PACKAGE_FOLDER_DEBUG "C:/Users/Administrator/.conan2/p/b/yaml-4260b6c618483/p")
set(yaml-cpp_BUILD_MODULES_PATHS_DEBUG )


set(yaml-cpp_INCLUDE_DIRS_DEBUG "${yaml-cpp_PACKAGE_FOLDER_DEBUG}/include")
set(yaml-cpp_RES_DIRS_DEBUG )
set(yaml-cpp_DEFINITIONS_DEBUG "-DYAML_CPP_STATIC_DEFINE")
set(yaml-cpp_SHARED_LINK_FLAGS_DEBUG )
set(yaml-cpp_EXE_LINK_FLAGS_DEBUG )
set(yaml-cpp_OBJECTS_DEBUG )
set(yaml-cpp_COMPILE_DEFINITIONS_DEBUG "YAML_CPP_STATIC_DEFINE")
set(yaml-cpp_COMPILE_OPTIONS_C_DEBUG )
set(yaml-cpp_COMPILE_OPTIONS_CXX_DEBUG )
set(yaml-cpp_LIB_DIRS_DEBUG "${yaml-cpp_PACKAGE_FOLDER_DEBUG}/lib")
set(yaml-cpp_BIN_DIRS_DEBUG )
set(yaml-cpp_LIBRARY_TYPE_DEBUG STATIC)
set(yaml-cpp_IS_HOST_WINDOWS_DEBUG 1)
set(yaml-cpp_LIBS_DEBUG yaml-cppd)
set(yaml-cpp_SYSTEM_LIBS_DEBUG )
set(yaml-cpp_FRAMEWORK_DIRS_DEBUG )
set(yaml-cpp_FRAMEWORKS_DEBUG )
set(yaml-cpp_BUILD_DIRS_DEBUG )
set(yaml-cpp_NO_SONAME_MODE_DEBUG FALSE)


# COMPOUND VARIABLES
set(yaml-cpp_COMPILE_OPTIONS_DEBUG
    "$<$<COMPILE_LANGUAGE:CXX>:${yaml-cpp_COMPILE_OPTIONS_CXX_DEBUG}>"
    "$<$<COMPILE_LANGUAGE:C>:${yaml-cpp_COMPILE_OPTIONS_C_DEBUG}>")
set(yaml-cpp_LINKER_FLAGS_DEBUG
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${yaml-cpp_SHARED_LINK_FLAGS_DEBUG}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${yaml-cpp_SHARED_LINK_FLAGS_DEBUG}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${yaml-cpp_EXE_LINK_FLAGS_DEBUG}>")


set(yaml-cpp_COMPONENTS_DEBUG )