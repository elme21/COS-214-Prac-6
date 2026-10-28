/**
 * @file PriorityDecorator.h
 * @brief Decorator that raises the priority reported for a task.
 * @note Pending: needs getPriority() added to the shared WorkComponent contract.
 */
#ifndef PRIORITYDECORATOR_H
#define PRIORITYDECORATOR_H

#include "WorkComponentDecorator.h"

/**
 * @brief Raises the priority reported for a task (e.g. late applications).
 *
 * With stacked priority decorators, the highest level wins. Ordering and
 * routing code can read the result through WorkComponent::getPriority()
 * without any type checks.
 *
 * @par Design pattern
 * Decorator (ConcreteDecorator role).
 */
class PriorityDecorator : public WorkComponentDecorator {
public:
    /**
     * @brief Wraps @p component with a priority level.
     * @param component Component to wrap. The decorator takes ownership.
     * @param level Priority to report; higher means more urgent.
     */
    PriorityDecorator(WorkComponent* component, int level);

    /** @return The larger of this level and the wrapped component's priority. */
    int getPriority() const override;

    /** @return A new decorator, owned by the caller, wrapping a deep copy. */
    WorkComponent* clone() const override;

private:
    int level;  ///< Priority reported by this decorator.
};

#endif
