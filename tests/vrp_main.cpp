#include "standard_csv_reader.h"

Scenario *loading_scenario(const std::string &case_file_path)
{
    StandardCsvReader reader = StandardCsvReader(case_file_path);
    Scenario *scenario = reader.loading_scenario();
    return scenario;
}

int main()
{
    // loading scenario
    const std::string case_file_path = "/Users/liq/worksapce/code_l/yunpy/test_case/case1";
    Scenario *scenario = loading_scenario(case_file_path);
    // cerate plan
    
    delete scenario;
    return 0;
}
