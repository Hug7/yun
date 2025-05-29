########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

list(APPEND spdlog_COMPONENT_NAMES spdlog::spdlog)
list(REMOVE_DUPLICATES spdlog_COMPONENT_NAMES)
if(DEFINED spdlog_FIND_DEPENDENCY_NAMES)
  list(APPEND spdlog_FIND_DEPENDENCY_NAMES fmt)
  list(REMOVE_DUPLICATES spdlog_FIND_DEPENDENCY_NAMES)
else()
  set(spdlog_FIND_DEPENDENCY_NAMES fmt)
endif()
set(fmt_FIND_MODE "NO_MODULE")

########### VARIABLES #######################################################################
#############################################################################################
set(spdlog_PACKAGE_FOLDER_DEBUG "C:/Users/Administrator/.conan2/p/b/spdlo872c9f72bbbd7/p")
set(spdlog_BUILD_MODULES_PATHS_DEBUG )


set(spdlog_INCLUDE_DIRS_DEBUG "${spdlog_PACKAGE_FOLDER_DEBUG}/include")
set(spdlog_RES_DIRS_DEBUG )
set(spdlog_DEFINITIONS_DEBUG "-DSPDLOG_FMT_EXTERNAL"
			"-DSPDLOG_COMPILED_LIB")
set(spdlog_SHARED_LINK_FLAGS_DEBUG )
set(spdlog_EXE_LINK_FLAGS_DEBUG )
set(spdlog_OBJECTS_DEBUG )
set(spdlog_COMPILE_DEFINITIONS_DEBUG "SPDLOG_FMT_EXTERNAL"
			"SPDLOG_COMPILED_LIB")
set(spdlog_COMPILE_OPTIONS_C_DEBUG )
set(spdlog_COMPILE_OPTIONS_CXX_DEBUG )
set(spdlog_LIB_DIRS_DEBUG "${spdlog_PACKAGE_FOLDER_DEBUG}/lib")
set(spdlog_BIN_DIRS_DEBUG )
set(spdlog_LIBRARY_TYPE_DEBUG STATIC)
set(spdlog_IS_HOST_WINDOWS_DEBUG 1)
set(spdlog_LIBS_DEBUG spdlogd)
set(spdlog_SYSTEM_LIBS_DEBUG )
set(spdlog_FRAMEWORK_DIRS_DEBUG )
set(spdlog_FRAMEWORKS_DEBUG )
set(spdlog_BUILD_DIRS_DEBUG )
set(spdlog_NO_SONAME_MODE_DEBUG FALSE)


# COMPOUND VARIABLES
set(spdlog_COMPILE_OPTIONS_DEBUG
    "$<$<COMPILE_LANGUAGE:CXX>:${spdlog_COMPILE_OPTIONS_CXX_DEBUG}>"
    "$<$<COMPILE_LANGUAGE:C>:${spdlog_COMPILE_OPTIONS_C_DEBUG}>")
set(spdlog_LINKER_FLAGS_DEBUG
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${spdlog_SHARED_LINK_FLAGS_DEBUG}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${spdlog_SHARED_LINK_FLAGS_DEBUG}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${spdlog_EXE_LINK_FLAGS_DEBUG}>")


set(spdlog_COMPONENTS_DEBUG spdlog::spdlog)
########### COMPONENT spdlog::spdlog VARIABLES ############################################

set(spdlog_spdlog_spdlog_INCLUDE_DIRS_DEBUG "${spdlog_PACKAGE_FOLDER_DEBUG}/include")
set(spdlog_spdlog_spdlog_LIB_DIRS_DEBUG "${spdlog_PACKAGE_FOLDER_DEBUG}/lib")
set(spdlog_spdlog_spdlog_BIN_DIRS_DEBUG )
set(spdlog_spdlog_spdlog_LIBRARY_TYPE_DEBUG STATIC)
set(spdlog_spdlog_spdlog_IS_HOST_WINDOWS_DEBUG 1)
set(spdlog_spdlog_spdlog_RES_DIRS_DEBUG )
set(spdlog_spdlog_spdlog_DEFINITIONS_DEBUG "-DSPDLOG_FMT_EXTERNAL"
			"-DSPDLOG_COMPILED_LIB")
set(spdlog_spdlog_spdlog_OBJECTS_DEBUG )
set(spdlog_spdlog_spdlog_COMPILE_DEFINITIONS_DEBUG "SPDLOG_FMT_EXTERNAL"
			"SPDLOG_COMPILED_LIB")
set(spdlog_spdlog_spdlog_COMPILE_OPTIONS_C_DEBUG "")
set(spdlog_spdlog_spdlog_COMPILE_OPTIONS_CXX_DEBUG "")
set(spdlog_spdlog_spdlog_LIBS_DEBUG spdlogd)
set(spdlog_spdlog_spdlog_SYSTEM_LIBS_DEBUG )
set(spdlog_spdlog_spdlog_FRAMEWORK_DIRS_DEBUG )
set(spdlog_spdlog_spdlog_FRAMEWORKS_DEBUG )
set(spdlog_spdlog_spdlog_DEPENDENCIES_DEBUG fmt::fmt)
set(spdlog_spdlog_spdlog_SHARED_LINK_FLAGS_DEBUG )
set(spdlog_spdlog_spdlog_EXE_LINK_FLAGS_DEBUG )
set(spdlog_spdlog_spdlog_NO_SONAME_MODE_DEBUG FALSE)

# COMPOUND VARIABLES
set(spdlog_spdlog_spdlog_LINKER_FLAGS_DEBUG
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${spdlog_spdlog_spdlog_SHARED_LINK_FLAGS_DEBUG}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${spdlog_spdlog_spdlog_SHARED_LINK_FLAGS_DEBUG}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${spdlog_spdlog_spdlog_EXE_LINK_FLAGS_DEBUG}>
)
set(spdlog_spdlog_spdlog_COMPILE_OPTIONS_DEBUG
    "$<$<COMPILE_LANGUAGE:CXX>:${spdlog_spdlog_spdlog_COMPILE_OPTIONS_CXX_DEBUG}>"
    "$<$<COMPILE_LANGUAGE:C>:${spdlog_spdlog_spdlog_COMPILE_OPTIONS_C_DEBUG}>")