#ifndef TASKROUTER_H
#define TASKROUTER_H

#include "AssignmentStrategy.h"

class WorkItem;

/**
 * Strategy pattern - Context.
 * Holds the current AssignmentStrategy (aggregation - it does not own it) and
 * delegates route(item) to it without knowing which concrete rule it is.
 */
class TaskRouter {
public:
    explicit TaskRouter(AssignmentStrategy* strategy = nullptr);

    /** Switch the assignment rule at runtime. */
    void setStrategy(AssignmentStrategy* strategy);
    AssignmentStrategy* getStrategy() const;

    /**
     * Assign 'item' using the current strategy.
     * @throws std::invalid_argument if item is null
     * @throws std::logic_error if no strategy has been set
     */
    void route(WorkItem* item);

private:
    AssignmentStrategy* strategy;
};

#endif
