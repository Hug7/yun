# Conan writes GTestConfig.cmake into CMAKE_BINARY_DIR; force this path so
# stale caches with GTest_DIR-NOTFOUND are repaired.
# set(GTest_DIR "${CMAKE_BINARY_DIR}" CACHE PATH "" FORCE)
set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR} ${CMAKE_PREFIX_PATH})
find_package(GTest CONFIG REQUIRED)

add_executable(vrp_exec vrp_main.cpp)
target_link_libraries(vrp_exec PRIVATE yun_solver)

add_executable(algo_alns_sa_test algorithm/algo_alns_sa_test.cpp)
target_link_libraries(algo_alns_sa_test PRIVATE yun_algorithm GTest::gtest_main)
add_test(NAME algo_alns_sa_test COMMAND algo_alns_sa_test)

add_executable(algo_alns_operator_test algorithm/algo_alns_operator_test.cpp)
target_link_libraries(algo_alns_operator_test PRIVATE yun_algorithm GTest::gtest_main)
add_test(NAME algo_alns_operator_test COMMAND algo_alns_operator_test)

add_executable(algo_alns_ruin_worst_removal_test algorithm/algo_alns_ruin_worst_removal_test.cpp)
target_link_libraries(algo_alns_ruin_worst_removal_test PRIVATE
    yun_algorithm GTest::gtest_main)
add_test(NAME algo_alns_ruin_worst_removal_test COMMAND algo_alns_ruin_worst_removal_test)

add_executable(algo_alns_ruin_shaw_removal_test
    algorithm/algo_alns_ruin_shaw_removal_test.cpp)
target_link_libraries(algo_alns_ruin_shaw_removal_test PRIVATE
    yun_algorithm GTest::gtest_main)
add_test(NAME algo_alns_ruin_shaw_removal_test COMMAND algo_alns_ruin_shaw_removal_test)

add_executable(algo_ls_knn_prune_test algorithm/algo_ls_knn_prune_test.cpp)
target_link_libraries(algo_ls_knn_prune_test PRIVATE yun_algorithm GTest::gtest_main)
add_test(NAME algo_ls_knn_prune_test COMMAND algo_ls_knn_prune_test)

add_executable(algo_ls_operator_test algorithm/algo_ls_operator_test.cpp)
target_link_libraries(algo_ls_operator_test PRIVATE yun_algorithm GTest::gtest_main)
add_test(NAME algo_ls_operator_test COMMAND algo_ls_operator_test)

add_executable(algo_ls_dist_prune_test algorithm/algo_ls_dist_prune_test.cpp)
target_link_libraries(algo_ls_dist_prune_test PRIVATE yun_algorithm GTest::gtest_main)
add_test(NAME algo_ls_dist_prune_test COMMAND algo_ls_dist_prune_test)

add_executable(stra_construct_config_test solver/stra_construct_config_test.cpp)
target_link_libraries(stra_construct_config_test PRIVATE
    yun_solver sol2::sol2 GTest::gtest_main)
add_test(NAME stra_construct_config_test COMMAND stra_construct_config_test)

add_executable(bd_time_window_utils_test scenario/bd_time_window_utils_test.cpp)
target_link_libraries(bd_time_window_utils_test PRIVATE yun_scenario GTest::gtest_main)
add_test(NAME bd_time_window_utils_test COMMAND bd_time_window_utils_test)

add_executable(bd_bitset_test scenario/bd_bitset_test.cpp)
target_link_libraries(bd_bitset_test PRIVATE yun_scenario GTest::gtest_main)
add_test(NAME bd_bitset_test COMMAND bd_bitset_test)

add_executable(bd_dist_matrix_test scenario/bd_dist_matrix_test.cpp)
target_link_libraries(bd_dist_matrix_test PRIVATE yun_scenario GTest::gtest_main)
add_test(NAME bd_dist_matrix_test COMMAND bd_dist_matrix_test)

add_executable(c_log_test common/c_log_test.cpp)
target_link_libraries(c_log_test PRIVATE yun_common GTest::gtest_main)
add_test(NAME c_log_test COMMAND c_log_test)

add_executable(c_time_utils_test common/c_time_utils_test.cpp)
target_link_libraries(c_time_utils_test PRIVATE yun_common GTest::gtest_main)
add_test(NAME c_time_utils_test COMMAND c_time_utils_test)

add_executable(pdm_node_test problem/pdm_node_test.cpp)
target_link_libraries(pdm_node_test PRIVATE yun_problem GTest::gtest_main)
add_test(NAME pdm_node_test COMMAND pdm_node_test)

add_executable(pdm_time_window_cache_test problem/pdm_time_window_cache_test.cpp)
target_link_libraries(pdm_time_window_cache_test PRIVATE yun_problem GTest::gtest_main)
add_test(NAME pdm_time_window_cache_test COMMAND pdm_time_window_cache_test)

add_executable(hc_label_value_test problem/hc_label_value_test.cpp)
target_link_libraries(hc_label_value_test PRIVATE yun_problem GTest::gtest_main)
add_test(NAME hc_label_value_test COMMAND hc_label_value_test)

add_executable(hc_precheck_test problem/hc_precheck_test.cpp)
target_link_libraries(hc_precheck_test PRIVATE yun_problem GTest::gtest_main)
add_test(NAME hc_precheck_test COMMAND hc_precheck_test)

add_executable(prob_pattern_spmd_test problem/prob_pattern_spmd_test.cpp)
target_link_libraries(prob_pattern_spmd_test PRIVATE yun_problem GTest::gtest_main)
add_test(NAME prob_pattern_spmd_test COMMAND prob_pattern_spmd_test)

add_executable(visual_manager_test problem/visual_manager_test.cpp)
target_link_libraries(visual_manager_test PRIVATE yun_problem GTest::gtest_main)
add_test(NAME visual_manager_test COMMAND visual_manager_test)

add_executable(charge_manager_test scenario/charge_manager_test.cpp)
target_link_libraries(charge_manager_test PRIVATE yun_scenario GTest::gtest_main)
target_compile_definitions(charge_manager_test PRIVATE
    YUN_CHARGE_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data/charge_case"
    YUN_CHARGE_OVERLAP_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data/charge_case_overlap")
add_test(NAME charge_manager_test COMMAND charge_manager_test)

add_executable(libfork_smoke_test algorithm/libfork_smoke_test.cpp)
target_link_libraries(libfork_smoke_test PRIVATE yun_algorithm GTest::gtest_main)
add_test(NAME libfork_smoke_test COMMAND libfork_smoke_test)

add_executable(csv_write_demo common/csv_write_demo.cpp)
target_link_libraries(csv_write_demo PRIVATE yun_common)

# DSL demo: lua 虚拟机 + sol2 绑定层
find_package(lua CONFIG REQUIRED)
find_package(sol2 CONFIG REQUIRED)

add_executable(lua_dsl_demo lua/lua_dsl_demo.cpp)
target_link_libraries(lua_dsl_demo PRIVATE sol2::sol2 lua::lua)
# 脚本目录通过编译宏注入，避免依赖运行时的工作目录
target_compile_definitions(lua_dsl_demo PRIVATE YUN_LUA_DEMO_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

# repair matrix 重构: 纯函数排序测试 + 基于内存 Problem 夹具的确定性/黄金值测试
add_executable(algo_alns_repair_rank_test algorithm/algo_alns_repair_rank_test.cpp)
target_link_libraries(algo_alns_repair_rank_test PRIVATE yun_algorithm GTest::gtest_main)
add_test(NAME algo_alns_repair_rank_test COMMAND algo_alns_repair_rank_test)

add_executable(algo_alns_repair_matrix_test algorithm/algo_alns_repair_matrix_test.cpp)
target_link_libraries(algo_alns_repair_matrix_test PRIVATE yun_algorithm GTest::gtest_main)
add_test(NAME algo_alns_repair_matrix_test COMMAND algo_alns_repair_matrix_test)
