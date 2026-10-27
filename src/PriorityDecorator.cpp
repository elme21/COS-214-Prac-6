#include "PriorityDecorator.h"
#include <algorithm>

PriorityDecorator::PriorityDecorator(WorkComponent* component, int lvl)
    : WorkComponentDecorator(component), level(lvl) {}

int PriorityDecorator::getPriority() const {
    return std::max(level, WorkComponentDecorator::getPriority());
}

WorkComponent* PriorityDecorator::clone() const {
    return new PriorityDecorator(*this);
}
