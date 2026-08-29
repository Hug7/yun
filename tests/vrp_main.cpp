#include "s_solver.h"



int main()
{
    const std::string case_file_path = "/Users/liq/worksapce/code_l/yunpy/test_case/case1";
    // create solver
    Solver* solver = new Solver(case_file_path);
    // loading scenario
    solver->load_scenario();
    // loading parameters
    solver->load_parameter();
    // create problem
    solver->create_problem();
    // base run
    solver->precheck();


    // cerate plan
    
    delete solver;
    return 0;
}
