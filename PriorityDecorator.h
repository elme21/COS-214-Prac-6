#ifndef PRIORITYDECORATOR_H
#define PRIORITYDECORATOR_H

#include "WorkComponentDecorator.h"

// Raises the priority reported for a task (e.g. late applications).
// With stacked priorities the highest level wins.
class PriorityDecorator : public WorkComponentDecorator {
public:
    PriorityDecorator(WorkComponent* component, int level);

    int getPriority() const override;
    WorkComponent* clone() const override;

private:
    int level;
};

#endif