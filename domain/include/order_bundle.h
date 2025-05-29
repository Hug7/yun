#ifndef ORDER_BUNDLE_H
#define ORDER_BUNDLE_H

#include <string>
#include <vector>

#include "order.h"


class OrderBundle
{
public:
    std::string id;
    std::vector<Order *> orders;
    bool fixed_seq;
    
};


#endif // ORDER_BUNDLE_H
