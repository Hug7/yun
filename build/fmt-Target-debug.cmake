# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(fmt_FRAMEWORKS_FOUND_DEBUG "") # Will be filled later
conan_find_apple_frameworks(fmt_FRAMEWORKS_FOUND_DEBUG "${fmt_FRAMEWORKS_DEBUG}" "${fmt_FRAMEWORK_DIRS_DEBUG}")

set(fmt_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET fmt_DEPS_TARGET)
    add_library(fmt_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET fmt_DEPS_TARGET
             APPEND PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Debug>:${fmt_FRAMEWORKS_FOUND_DEBUG}>
             $<$<CONFIG:Debug>:${fmt_SYSTEM_LIBS_DEBUG}>
             $<$<CONFIG:Debug>:>)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### fmt_DEPS_TARGET to all of them
conan_package_library_targets("${fmt_LIBS_DEBUG}"    # libraries
                              "${fmt_LIB_DIRS_DEBUG}" # package_libdir
                              "${fmt_BIN_DIRS_DEBUG}" # package_bindir
                              "${fmt_LIBRARY_TYPE_DEBUG}"
                              "${fmt_IS_HOST_WINDOWS_DEBUG}"
                              fmt_DEPS_TARGET
                              fmt_LIBRARIES_TARGETS  # out_libraries_targets
                              "_DEBUG"
                              "fmt"    # package_name
                              "${fmt_NO_SONAME_MODE_DEBUG}")  # soname

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${fmt_BUILD_DIRS_DEBUG} ${CMAKE_MODULE_PATH})

########## COMPONENTS TARGET PROPERTIES Debug ########################################

    ########## COMPONENT fmt::fmt #############

        set(fmt_fmt_fmt_FRAMEWORKS_FOUND_DEBUG "")
        conan_find_apple_frameworks(fmt_fmt_fmt_FRAMEWORKS_FOUND_DEBUG "${fmt_fmt_fmt_FRAMEWORKS_DEBUG}" "${fmt_fmt_fmt_FRAMEWORK_DIRS_DEBUG}")

        set(fmt_fmt_fmt_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET fmt_fmt_fmt_DEPS_TARGET)
            add_library(fmt_fmt_fmt_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET fmt_fmt_fmt_DEPS_TARGET
                     APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Debug>:${fmt_fmt_fmt_FRAMEWORKS_FOUND_DEBUG}>
                     $<$<CONFIG:Debug>:${fmt_fmt_fmt_SYSTEM_LIBS_DEBUG}>
                     $<$<CONFIG:Debug>:${fmt_fmt_fmt_DEPENDENCIES_DEBUG}>
                     )

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'fmt_fmt_fmt_DEPS_TARGET' to all of them
        conan_package_library_targets("${fmt_fmt_fmt_LIBS_DEBUG}"
                              "${fmt_fmt_fmt_LIB_DIRS_DEBUG}"
                              "${fmt_fmt_fmt_BIN_DIRS_DEBUG}" # package_bindir
                              "${fmt_fmt_fmt_LIBRARY_TYPE_DEBUG}"
                              "${fmt_fmt_fmt_IS_HOST_WINDOWS_DEBUG}"
                              fmt_fmt_fmt_DEPS_TARGET
                              fmt_fmt_fmt_LIBRARIES_TARGETS
                              "_DEBUG"
                              "fmt_fmt_fmt"
                              "${fmt_fmt_fmt_NO_SONAME_MODE_DEBUG}")


        ########## TARGET PROPERTIES #####################################
        set_property(TARGET fmt::fmt
                     APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Debug>:${fmt_fmt_fmt_OBJECTS_DEBUG}>
                     $<$<CONFIG:Debug>:${fmt_fmt_fmt_LIBRARIES_TARGETS}>
                     )

        if("${fmt_fmt_fmt_LIBS_DEBUG}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET fmt::fmt
                         APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                         fmt_fmt_fmt_DEPS_TARGET)
        endif()

        set_property(TARGET fmt::fmt APPEND PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Debug>:${fmt_fmt_fmt_LINKER_FLAGS_DEBUG}>)
        set_property(TARGET fmt::fmt APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Debug>:${fmt_fmt_fmt_INCLUDE_DIRS_DEBUG}>)
        set_property(TARGET fmt::fmt APPEND PROPERTY INTERFACE_LINK_DIRECTORIES
                     $<$<CONFIG:Debug>:${fmt_fmt_fmt_LIB_DIRS_DEBUG}>)
        set_property(TARGET fmt::fmt APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Debug>:${fmt_fmt_fmt_COMPILE_DEFINITIONS_DEBUG}>)
        set_property(TARGET fmt::fmt APPEND PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Debug>:${fmt_fmt_fmt_COMPILE_OPTIONS_DEBUG}>)


    ########## AGGREGATED GLOBAL TARGET WITH THE COMPONENTS #####################
    set_property(TARGET fmt::fmt APPEND PROPERTY INTERFACE_LINK_LIBRARIES fmt::fmt)

########## For the modules (FindXXX)
set(fmt_LIBRARIES_DEBUG fmt::fmt)
