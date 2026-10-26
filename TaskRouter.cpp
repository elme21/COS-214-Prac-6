#include "TaskRouter.h"
#include <stdexcept>

TaskRouter::TaskRouter(AssignmentStrategy* s) : strategy(s) {}

void TaskRouter::setStrategy(AssignmentStrategy* s) { strategy = s; }

AssignmentStrategy* TaskRouter::getStrategy() const { return strategy; }

void TaskRouter::route(WorkItem* item) {
    if (!item)     throw std::invalid_argument("TaskRouter::route: item is null");
    if (!strategy) throw std::logic_error("TaskRouter::route: no AssignmentStrategy set");
    strategy->route(item);
}