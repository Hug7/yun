/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pr_problem.h"
#include "hc_location.h"
#include "hc_vehicle.h"
#include "cc_dist.h"

// ====== implement of Problem ======
Problem::Problem(const Scenario *scenario) : scenario(scenario)
{
    switch (scenario->parameter->pick_drop_pattern)
    {
    case PickDropPatternType::SPMD:
        this->pd_pattern = new SPMD(scenario);
        break;
    case PickDropPatternType::MPMD:
        throw std::invalid_argument("unsupported pick drop pattern!");
        break;
    case PickDropPatternType::PDP:
        throw std::invalid_argument("unsupported pick drop pattern!");
        break;
    default:
        throw std::invalid_argument("invalid pick drop pattern!");
        break;
    }

    const Parameter *parameter = scenario->parameter;

    this->hc_manager = new HardConstraintManager();
    this->sc_manager = new SoftConstraintManager();
    this->cc_manager = new CostConstraintManager();

    // hard constraints: base
    this->hc_manager->add_constr(new HcVehicleCapacity());
    // todo add hard constraints by scenario

    // == hard constraints: max drop node count
    if (parameter->max_pick_node_count > HardConstraintParameter::DEFAULT_MAX_PICK_NODE_COUNT)
    {
        this->hc_manager->add_constr(new HcMaxPickNodeCount(parameter->max_pick_node_count));
    }
    // == hard constraints: max pick node count
    if (parameter->max_drop_node_count > HardConstraintParameter::DEFAULT_MAX_DROP_NODE_COUNT)
    {
        this->hc_manager->add_constr(new HcMaxDropNodeCount(parameter->max_drop_node_count));
    }

    // cost constraints
    // == cost constraints: dist
    if (parameter->cc_constr_dist_factor > CostConstraintParameter::CC_DIST_DEFAULT_DIST_FACTOR)
    {
        this->cc_manager->add_constr(new CcDist(parameter->cc_constr_dist_factor));
    }
}


void Problem::eval_load(Load *load)
{
    // todo 是否需要重置待讨论
    LoadConstrProfile::UPtr &constr_profile = load->constr_profile;
    constr_profile->reset();

    this->hc_manager->eval_constrs(load, constr_profile);
    
    this->cc_manager->eval_constrs(load, constr_profile);

    this->sc_manager->eval_constrs(load, constr_profile);
    // update obj val
    constr_profile->update_obj_val();
}

LoadConstrProfile::UPtr Problem::tmp_eval_load(Load *load)
{
    LoadConstrProfile::UPtr constr_profile = std::make_unique<LoadConstrProfile>();
    this->hc_manager->eval_constrs(load, constr_profile);
    if (constr_profile->infesible)
    {
        return constr_profile;
    }

    this->cc_manager->eval_constrs(load, constr_profile);
    if (constr_profile->infesible)
    {
        return constr_profile;
    }

    this->sc_manager->eval_constrs(load, constr_profile);

    // update obj val
    constr_profile->update_obj_val();

    return constr_profile;
}
