#include "location.h"

Location::Location(std::string id) : id(id)
{
    this->work_time = 0;
    this->first_visit = false;
    this->last_visit = false;
}

Location::~Location()
{
}
