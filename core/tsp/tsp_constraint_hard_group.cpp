#include "tsp_constraint_hard.h"

bool TspConstraintHardGroup::checkMoveAction(int route_ind_a, int route_ind_b, std::unique_ptr<TspSolutionRoute> &route, TspContext *&tsp_context)
{
    /**
     * if a - b <= 1
     *   do swap
     * if a > b
     *   b-1,b,b+1,...,a-1,a,a+1 -> b-1,a,b,b+1,...,a-1,a+1
     *   b-1,b,b+1(a-1),a,a+1 -> b-1,a,b,b+1(a-1),a+1
     * if a < b
     *   a-1,a,a+1,...,b-1,b,b+1 -> a-1,a+1,...,b-1,b,a,b+1
     *   a-1,a,a+1(b-1),b,b+1 -> a-1,a+1(b-1),b,a,b+1
     */
    if (std::abs(route_ind_a - route_ind_b) <= 1)
    {
        return this->checkSwapAction(route_ind_a, route_ind_b, route, tsp_context);
    }
    TspLocation *&loc_a = tsp_context->locations[route->nodes[route_ind_a]->loc_ind];
    TspLocation *&loc_b = tsp_context->locations[route->nodes[route_ind_b]->loc_ind];

    // locations all are same group mark (default or normal)
    if (loc_a->group_mark == loc_b->group_mark)
    {
        if (loc_a->is_default_group_mark)
        {
            return true;
        }
        // check group priority
        if (route_ind_a > route_ind_b && loc_a->group_priority > loc_b->group_priority)
        {
            return false;
        }
        else if (route_ind_a < route_ind_b && loc_a->group_priority < loc_b->group_priority)
        {
            return false;
        }
        return true;
    }
    // location a is non default is vaild (single group mark is non existent)
    if (!loc_a->is_default_group_mark)
    {
        return false;
    }
    // current location a must is default group mark
    if (route_ind_a > route_ind_b)
    {
        // check: left side of loction b can be insert
        return loc_b->group_mark != tsp_context->locations[route->nodes[route_ind_b - 1]->loc_ind]->group_mark;
    }
    else
    {
        // check: right side of loction b can be insert
        return loc_b->group_mark != tsp_context->locations[route->nodes[route_ind_b + 1]->loc_ind]->group_mark;
    }

    return true;
}

bool TspConstraintHardGroup::checkSwapAction(int route_ind_a, int route_ind_b, std::unique_ptr<TspSolutionRoute> &route, TspContext *&tsp_context)
{
    /**
     * a-1,a,a+1,b-1,b,b+1 -> a-1,b,a+1,b-1,a,b+1
     * a-1,a,a+1(b-1),b,b+1 -> a-1,b,a+1(b-1),a,b+1
     * a-1,a,b,b+1 -> a-1,b,a,b+1
     */
    if (route_ind_a > route_ind_b)
    {
        int tmp = route_ind_a;
        route_ind_a = route_ind_b;
        route_ind_b = tmp;
    }

    TspLocation *&loc_a = tsp_context->locations[route->nodes[route_ind_a]->loc_ind];
    TspLocation *&loc_b = tsp_context->locations[route->nodes[route_ind_b]->loc_ind];

    // locations all are same group mark (default or normal)
    if (loc_a->group_mark == loc_b->group_mark)
    {
        if (loc_a->is_default_group_mark)
        {
            return true;
        }
        // check group priority
        if (loc_a->group_priority < loc_b->group_priority)
        {
            return false;
        }
        return true;
    }
    // location both is non default group mark
    if (!loc_a->is_default_group_mark && !loc_a->is_default_group_mark)
    {
        return false;
    }
    // discuss respectively
    if (loc_a->is_default_group_mark)
    {
        // loc_a is default group mark
        // loc_b is non default group mark
        TspLocation *&loc_next_b = tsp_context->locations[route->nodes[route_ind_b + 1]->loc_ind];
        if (loc_b->group_mark == loc_next_b->group_mark)
        {
            return false;
        }
        // currnet location b in rightmost of group mark[location b]
        TspLocation *&loc_next_a = tsp_context->locations[route->nodes[route_ind_a + 1]->loc_ind];
        if (loc_next_a->group_mark != loc_b->group_mark)
        {
            return false;
        }
        else if (loc_b->group_priority > loc_next_a->group_priority)
        {
            return false;
        }
        return true;
    }
    else if (loc_b->is_default_group_mark)
    {
        // loc_a is non default group mark
        // loc_b is default group mark
        TspLocation *&loc_pre_a = tsp_context->locations[route->nodes[route_ind_a - 1]->loc_ind];
        if (loc_a->group_mark == loc_pre_a->group_mark)
        {
            return false;
        }
        // currnet location a in leftmost of group mark[location a]
        TspLocation *&loc_pre_b = tsp_context->locations[route->nodes[route_ind_b - 1]->loc_ind];
        if (loc_pre_b->group_mark != loc_a->group_mark)
        {
            return false;
        }
        else if (loc_a->group_priority < loc_pre_b->group_priority)
        {
            return false;
        }
        return true;
    }
    else
    {
        throw std::runtime_error("logic loss of group constraint checkSwapAction");
    }

    return true;
}

bool TspConstraintHardGroup::checkKoptAction(int begin_route_ind, int end_route_ind, std::vector<std::tuple<int, int>> &part_route_indices, std::unique_ptr<TspSolutionRoute> &route, TspContext *&tsp_context)
{
    std::vector<std::unique_ptr<TspSolutionNode>> &nodes = route->nodes;
    std::vector<TspLocation *> &locations = tsp_context->locations;

    std::unordered_set<std::string> visited_group_mark;
    std::string pre_group_mark = locations[nodes[begin_route_ind - 1]->loc_ind]->group_mark;
    int pre_group_priority = locations[nodes[begin_route_ind - 1]->loc_ind]->group_priority;
    bool pre_group_mark_is_default = locations[nodes[begin_route_ind - 1]->loc_ind]->is_default_group_mark;

    int left_route_ind, right_route_ind;
    for (auto &part_route_ind : part_route_indices)
    {
        std::tie(left_route_ind, right_route_ind) = part_route_ind;
        if (left_route_ind <= right_route_ind)
        {
            for (int u = left_route_ind; u <= right_route_ind; u++)
            {
                TspLocation *&cur_loc = locations[nodes[u]->loc_ind];
                std::string cur_group_mark = cur_loc->group_mark;
                if (visited_group_mark.count(cur_group_mark))
                {
                    return false;
                }
                if (pre_group_mark_is_default)
                {
                    pre_group_mark = cur_group_mark;
                    pre_group_priority = cur_loc->group_priority;
                    pre_group_mark_is_default = cur_loc->is_default_group_mark;
                }
                else
                {
                    if (pre_group_mark == cur_group_mark)
                    {
                        if (cur_loc->group_priority < pre_group_priority)
                        {
                            return false;
                        }
                    }
                    else
                    {
                        visited_group_mark.insert(pre_group_mark);
                        pre_group_mark = cur_group_mark;
                        pre_group_priority = cur_loc->group_priority;
                        pre_group_mark_is_default = cur_loc->is_default_group_mark;
                    }
                }
            }
        }
        else
        {
            for (int u = left_route_ind; u >= right_route_ind; u--)
            {
                TspLocation *&cur_loc = locations[nodes[u]->loc_ind];
                std::string cur_group_mark = cur_loc->group_mark;
                if (visited_group_mark.count(cur_group_mark))
                {
                    return false;
                }
                if (pre_group_mark_is_default)
                {                    
                    pre_group_mark = cur_group_mark;
                    pre_group_priority = cur_loc->group_priority;
                    pre_group_mark_is_default = cur_loc->is_default_group_mark;
                }
                else
                {
                    if (pre_group_mark == cur_group_mark)
                    {
                        if (cur_loc->group_priority < pre_group_priority)
                        {
                            return false;
                        }
                    }
                    else
                    {
                        visited_group_mark.insert(pre_group_mark);
                        pre_group_mark = cur_group_mark;
                        pre_group_priority = cur_loc->group_priority;
                        pre_group_mark_is_default = cur_loc->is_default_group_mark;
                    }
                }
            }
        }
    }
    // 片段尾部是否满足约束
    TspLocation *&cur_loc = locations[nodes[end_route_ind + 1]->loc_ind];
    std::string cur_group_mark = cur_loc->group_mark;
    if (visited_group_mark.count(cur_group_mark))
    {
        return false;
    }
    if (pre_group_mark == cur_group_mark)
    {
        if (cur_loc->group_priority < pre_group_priority)
        {
            return false;
        }
    }

    return true;
}
