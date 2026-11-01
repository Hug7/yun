# SDK 打包清单：仅供 scripts/build_sdk.py 读取。
#
# 不使用 link.txt（Ninja/Windows 下不存在），也不要求构建 Python 绑定。
# 输出：
#   ${CMAKE_BINARY_DIR}/sdk-archives-<CONFIG>.txt   每行一个静态库绝对路径
#   ${CMAKE_BINARY_DIR}/sdk-includes-<CONFIG>.txt   每行一个头文件目录绝对路径

# 依赖的 imported target 是在各组件子目录里 find_package 出来的，
# imported target 默认只在所在目录及其子目录可见；这里在根作用域重新
# find_package 一次，让下面的遍历能拿到 fmt::fmt 等目标。
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
find_package(lua CONFIG REQUIRED)
find_package(sol2 CONFIG REQUIRED)
find_package(libfork CONFIG REQUIRED)

set(YUN_SDK_OWN_STATIC_TARGETS
    yun_solver
    yun_algorithm
    yun_problem
    yun_scenario)

set(YUN_SDK_INCLUDE_TARGETS
    fmt::fmt
    spdlog::spdlog
    yaml-cpp::yaml-cpp
    magic_enum::magic_enum
    sol2::sol2
    libfork::libfork
    lua::lua)

foreach(_target IN LISTS YUN_SDK_OWN_STATIC_TARGETS YUN_SDK_INCLUDE_TARGETS)
    if(NOT TARGET ${_target})
        message(FATAL_ERROR "YUN_BUILD_SDK: 找不到目标 ${_target}")
    endif()
endforeach()

# 展开 CMake 的 LINK_ONLY / CONFIG 包装，得到真正的目标名。
function(_yun_unwrap_genex out value)
    set(_value "${value}")
    while(TRUE)
        if(_value MATCHES "^\\$<LINK_ONLY:(.*)>$")
            set(_value "${CMAKE_MATCH_1}")
        elseif(_value MATCHES "^\\$<\\$<CONFIG:Release>:(.*)>$")
            set(_value "${CMAKE_MATCH_1}")
        elseif(_value MATCHES "^\\$<\\$<CONFIG:Debug>:(.*)>$")
            set(_value "")
        else()
            break()
        endif()
    endwhile()
    set(${out} "${_value}" PARENT_SCOPE)
endfunction()

# 从 target 的链接闭包里收集静态库的绝对路径。
# 只收集 imported static library（Conan 依赖）；yun 自己的 target 由调用方
# 用 $<TARGET_FILE:...> 在 generate 阶段展开。
function(_yun_collect_dependency_archives out)
    set(_queue ${ARGN})
    set(_seen "")
    set(_archives "")
    while(_queue)
        list(POP_FRONT _queue _target)
        if(_target IN_LIST _seen)
            continue()
        endif()
        list(APPEND _seen "${_target}")
        if(NOT TARGET "${_target}")
            continue()
        endif()
        get_target_property(_type "${_target}" TYPE)
        if(_type STREQUAL "STATIC_LIBRARY")
            get_target_property(_location "${_target}" IMPORTED_LOCATION_RELEASE)
            if(NOT _location)
                get_target_property(_location "${_target}" IMPORTED_LOCATION)
            endif()
            if(_location AND NOT _location MATCHES "-NOTFOUND$")
                list(APPEND _archives "${_location}")
            endif()
        endif()
        foreach(_property LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
            get_target_property(_links "${_target}" ${_property})
            if(NOT _links)
                continue()
            endif()
            foreach(_link IN LISTS _links)
                _yun_unwrap_genex(_link "${_link}")
                if(_link)
                    list(APPEND _queue "${_link}")
                endif()
            endforeach()
        endforeach()
    endwhile()
    list(REMOVE_DUPLICATES _archives)
    set(${out} "${_archives}" PARENT_SCOPE)
endfunction()

_yun_collect_dependency_archives(YUN_SDK_DEP_ARCHIVES yun_solver)

set(_own_archives "")
foreach(_target IN LISTS YUN_SDK_OWN_STATIC_TARGETS)
    string(APPEND _own_archives "$<TARGET_FILE:${_target}>\n")
endforeach()

set(_dep_archives "")
foreach(_archive IN LISTS YUN_SDK_DEP_ARCHIVES)
    string(APPEND _dep_archives "${_archive}\n")
endforeach()

file(GENERATE
    OUTPUT "${CMAKE_BINARY_DIR}/sdk-archives-$<CONFIG>.txt"
    CONTENT "${_own_archives}${_dep_archives}")

set(_includes "")
foreach(_target IN LISTS YUN_SDK_INCLUDE_TARGETS)
    string(APPEND _includes
        "$<JOIN:$<TARGET_PROPERTY:${_target},INTERFACE_INCLUDE_DIRECTORIES>,\n>\n")
endforeach()
foreach(_component common scenario problem algorithm solver)
    string(APPEND _includes "${PROJECT_SOURCE_DIR}/${_component}/include\n")
endforeach()

file(GENERATE
    OUTPUT "${CMAKE_BINARY_DIR}/sdk-includes-$<CONFIG>.txt"
    CONTENT "${_includes}")
