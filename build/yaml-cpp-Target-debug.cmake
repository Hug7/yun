# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(yaml-cpp_FRAMEWORKS_FOUND_DEBUG "") # Will be filled later
conan_find_apple_frameworks(yaml-cpp_FRAMEWORKS_FOUND_DEBUG "${yaml-cpp_FRAMEWORKS_DEBUG}" "${yaml-cpp_FRAMEWORK_DIRS_DEBUG}")

set(yaml-cpp_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET yaml-cpp_DEPS_TARGET)
    add_library(yaml-cpp_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET yaml-cpp_DEPS_TARGET
             APPEND PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Debug>:${yaml-cpp_FRAMEWORKS_FOUND_DEBUG}>
             $<$<CONFIG:Debug>:${yaml-cpp_SYSTEM_LIBS_DEBUG}>
             $<$<CONFIG:Debug>:>)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### yaml-cpp_DEPS_TARGET to all of them
conan_package_library_targets("${yaml-cpp_LIBS_DEBUG}"    # libraries
                              "${yaml-cpp_LIB_DIRS_DEBUG}" # package_libdir
                              "${yaml-cpp_BIN_DIRS_DEBUG}" # package_bindir
                              "${yaml-cpp_LIBRARY_TYPE_DEBUG}"
                              "${yaml-cpp_IS_HOST_WINDOWS_DEBUG}"
                              yaml-cpp_DEPS_TARGET
                              yaml-cpp_LIBRARIES_TARGETS  # out_libraries_targets
                              "_DEBUG"
                              "yaml-cpp"    # package_name
                              "${yaml-cpp_NO_SONAME_MODE_DEBUG}")  # soname

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${yaml-cpp_BUILD_DIRS_DEBUG} ${CMAKE_MODULE_PATH})

########## GLOBAL TARGET PROPERTIES Debug ########################################
    set_property(TARGET yaml-cpp::yaml-cpp
                 APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                 $<$<CONFIG:Debug>:${yaml-cpp_OBJECTS_DEBUG}>
                 $<$<CONFIG:Debug>:${yaml-cpp_LIBRARIES_TARGETS}>
                 )

    if("${yaml-cpp_LIBS_DEBUG}" STREQUAL "")
        # If the package is not declaring any "cpp_info.libs" the package deps, system libs,
        # frameworks etc are not linked to the imported targets and we need to do it to the
        # global target
        set_property(TARGET yaml-cpp::yaml-cpp
                     APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                     yaml-cpp_DEPS_TARGET)
    endif()

    set_property(TARGET yaml-cpp::yaml-cpp
                 APPEND PROPERTY INTERFACE_LINK_OPTIONS
                 $<$<CONFIG:Debug>:${yaml-cpp_LINKER_FLAGS_DEBUG}>)
    set_property(TARGET yaml-cpp::yaml-cpp
                 APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                 $<$<CONFIG:Debug>:${yaml-cpp_INCLUDE_DIRS_DEBUG}>)
    # Necessary to find LINK shared libraries in Linux
    set_property(TARGET yaml-cpp::yaml-cpp
                 APPEND PROPERTY INTERFACE_LINK_DIRECTORIES
                 $<$<CONFIG:Debug>:${yaml-cpp_LIB_DIRS_DEBUG}>)
    set_property(TARGET yaml-cpp::yaml-cpp
                 APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS
                 $<$<CONFIG:Debug>:${yaml-cpp_COMPILE_DEFINITIONS_DEBUG}>)
    set_property(TARGET yaml-cpp::yaml-cpp
                 APPEND PROPERTY INTERFACE_COMPILE_OPTIONS
                 $<$<CONFIG:Debug>:${yaml-cpp_COMPILE_OPTIONS_DEBUG}>)

########## For the modules (FindXXX)
set(yaml-cpp_LIBRARIES_DEBUG yaml-cpp::yaml-cpp)
