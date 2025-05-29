########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

list(APPEND fmt_COMPONENT_NAMES fmt::fmt)
list(REMOVE_DUPLICATES fmt_COMPONENT_NAMES)
if(DEFINED fmt_FIND_DEPENDENCY_NAMES)
  list(APPEND fmt_FIND_DEPENDENCY_NAMES )
  list(REMOVE_DUPLICATES fmt_FIND_DEPENDENCY_NAMES)
else()
  set(fmt_FIND_DEPENDENCY_NAMES )
endif()

########### VARIABLES #######################################################################
#############################################################################################
set(fmt_PACKAGE_FOLDER_DEBUG "C:/Users/Administrator/.conan2/p/b/fmt8e34431c0f16d/p")
set(fmt_BUILD_MODULES_PATHS_DEBUG )


set(fmt_INCLUDE_DIRS_DEBUG "${fmt_PACKAGE_FOLDER_DEBUG}/include")
set(fmt_RES_DIRS_DEBUG )
set(fmt_DEFINITIONS_DEBUG )
set(fmt_SHARED_LINK_FLAGS_DEBUG )
set(fmt_EXE_LINK_FLAGS_DEBUG )
set(fmt_OBJECTS_DEBUG )
set(fmt_COMPILE_DEFINITIONS_DEBUG )
set(fmt_COMPILE_OPTIONS_C_DEBUG )
set(fmt_COMPILE_OPTIONS_CXX_DEBUG )
set(fmt_LIB_DIRS_DEBUG "${fmt_PACKAGE_FOLDER_DEBUG}/lib")
set(fmt_BIN_DIRS_DEBUG )
set(fmt_LIBRARY_TYPE_DEBUG STATIC)
set(fmt_IS_HOST_WINDOWS_DEBUG 1)
set(fmt_LIBS_DEBUG fmtd)
set(fmt_SYSTEM_LIBS_DEBUG )
set(fmt_FRAMEWORK_DIRS_DEBUG )
set(fmt_FRAMEWORKS_DEBUG )
set(fmt_BUILD_DIRS_DEBUG )
set(fmt_NO_SONAME_MODE_DEBUG FALSE)


# COMPOUND VARIABLES
set(fmt_COMPILE_OPTIONS_DEBUG
    "$<$<COMPILE_LANGUAGE:CXX>:${fmt_COMPILE_OPTIONS_CXX_DEBUG}>"
    "$<$<COMPILE_LANGUAGE:C>:${fmt_COMPILE_OPTIONS_C_DEBUG}>")
set(fmt_LINKER_FLAGS_DEBUG
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${fmt_SHARED_LINK_FLAGS_DEBUG}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${fmt_SHARED_LINK_FLAGS_DEBUG}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${fmt_EXE_LINK_FLAGS_DEBUG}>")


set(fmt_COMPONENTS_DEBUG fmt::fmt)
########### COMPONENT fmt::fmt VARIABLES ############################################

set(fmt_fmt_fmt_INCLUDE_DIRS_DEBUG "${fmt_PACKAGE_FOLDER_DEBUG}/include")
set(fmt_fmt_fmt_LIB_DIRS_DEBUG "${fmt_PACKAGE_FOLDER_DEBUG}/lib")
set(fmt_fmt_fmt_BIN_DIRS_DEBUG )
set(fmt_fmt_fmt_LIBRARY_TYPE_DEBUG STATIC)
set(fmt_fmt_fmt_IS_HOST_WINDOWS_DEBUG 1)
set(fmt_fmt_fmt_RES_DIRS_DEBUG )
set(fmt_fmt_fmt_DEFINITIONS_DEBUG )
set(fmt_fmt_fmt_OBJECTS_DEBUG )
set(fmt_fmt_fmt_COMPILE_DEFINITIONS_DEBUG )
set(fmt_fmt_fmt_COMPILE_OPTIONS_C_DEBUG "")
set(fmt_fmt_fmt_COMPILE_OPTIONS_CXX_DEBUG "")
set(fmt_fmt_fmt_LIBS_DEBUG fmtd)
set(fmt_fmt_fmt_SYSTEM_LIBS_DEBUG )
set(fmt_fmt_fmt_FRAMEWORK_DIRS_DEBUG )
set(fmt_fmt_fmt_FRAMEWORKS_DEBUG )
set(fmt_fmt_fmt_DEPENDENCIES_DEBUG )
set(fmt_fmt_fmt_SHARED_LINK_FLAGS_DEBUG )
set(fmt_fmt_fmt_EXE_LINK_FLAGS_DEBUG )
set(fmt_fmt_fmt_NO_SONAME_MODE_DEBUG FALSE)

# COMPOUND VARIABLES
set(fmt_fmt_fmt_LINKER_FLAGS_DEBUG
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${fmt_fmt_fmt_SHARED_LINK_FLAGS_DEBUG}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${fmt_fmt_fmt_SHARED_LINK_FLAGS_DEBUG}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${fmt_fmt_fmt_EXE_LINK_FLAGS_DEBUG}>
)
set(fmt_fmt_fmt_COMPILE_OPTIONS_DEBUG
    "$<$<COMPILE_LANGUAGE:CXX>:${fmt_fmt_fmt_COMPILE_OPTIONS_CXX_DEBUG}>"
    "$<$<COMPILE_LANGUAGE:C>:${fmt_fmt_fmt_COMPILE_OPTIONS_C_DEBUG}>")