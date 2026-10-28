#ifndef TASKROUTER_H
#define TASKROUTER_H

#include "AssignmentStrategy.h"

class WorkItem;

/**
 * @file TaskRouter.h
 * @brief Context of the Strategy pattern.
 *
 * @details The router is the only class the rest of the engine talks to for assignment. It delegates
 * entirely to the current strategy, which is how the rule can change at runtime.
 *
 * @author Logan (Observer and Strategy)
 * @date 2026-10-28
 * @ingroup StrategyPattern
 */

class TaskRouter {
public:
    explicit TaskRouter(AssignmentStrategy* strategy = nullptr);

    /** Switch the assignment rule at runtime. */
    void setStrategy(AssignmentStrategy* strategy);
    AssignmentStrategy* getStrategy() const;

     /// @brief Assigns a work item using the current strategy.
    /// @param item the work item to assign (not owned)
    /// @return true if the item was assigned
    /// @throws std::invalid_argument if item is null
    /// @throws std::logic_error if no strategy has been set
    /// @see AssignmentStrategy::route()
    void route(WorkItem* item);

private:
    AssignmentStrategy* strategy;
};

#endif
