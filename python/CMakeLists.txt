find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)
find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(yun_python_binding WITH_SOABI yun_solver_binding.cpp)
target_link_libraries(yun_python_binding PRIVATE yun_solver)
set_target_properties(yun_python_binding PROPERTIES OUTPUT_NAME "_core")

install(TARGETS yun_python_binding
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
