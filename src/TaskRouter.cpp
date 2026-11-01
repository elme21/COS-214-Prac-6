/**
 * @file TaskRouter.cpp
 * @brief Implementation of TaskRouter.
 *
 * @details Validates its arguments (throwing on misuse) and then delegates to the strategy without inspecting it.
 *
 * @author Logan (Observer and Strategy)
 * @date 2026-10-28
 * 
 */
#include "TaskRouter.h"
#include <stdexcept>

TaskRouter::TaskRouter(AssignmentStrategy* s) : strategy(s) {}

void TaskRouter::setStrategy(AssignmentStrategy* s) { strategy = s; }

AssignmentStrategy* TaskRouter::getStrategy() const { return strategy; }

bool TaskRouter::route(WorkItem* item) {
    if (!item)     throw std::invalid_argument("TaskRouter::route: item is null");
    if (!strategy) throw std::logic_error("TaskRouter::route: no AssignmentStrategy set");
    return strategy->route(item);
}
