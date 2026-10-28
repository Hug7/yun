find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)
find_package(pybind11 CONFIG REQUIRED)

# target 名保持语言维度（yun_python_binding），与 Python 发行包名解耦；
# 将来 Java 绑定可新增独立的 yun_java_binding target，二者都链接 yun_solver。
pybind11_add_module(yun_python_binding WITH_SOABI yun_solver_binding.cpp)
target_link_libraries(yun_python_binding PRIVATE yun_solver)
set_target_properties(yun_python_binding PROPERTIES
    OUTPUT_NAME "_core"
    # 依赖均为静态链接，清除构建机 conan 缓存带来的 LC_RPATH
    INSTALL_RPATH ""
)

install(TARGETS yun_python_binding
    LIBRARY DESTINATION yunpyopt
    ARCHIVE DESTINATION yunpyopt
    RUNTIME DESTINATION yunpyopt
)
