#ifndef ASSIGNEDSTATE_H
#define ASSIGNEDSTATE_H

#include "WorkItemState.h"

/// @brief
/// The AssignedState class is a ConcreteState of the State pattern: a participant owns the work item
/// but has not started it. Allows start() -> InProgressState, escalate() -> EscalatedState and
/// cancel() -> CancelledState; every other operation is refused.
class AssignedState : public WorkItemState {
public:
    /// @brief gets the single shared instance of this stateless state
    /// @return the shared AssignedState instance
    static WorkItemState& instance();

    /// @brief starts work on the work item: moves it to InProgressState
    /// @param item the work item whose state is changing
    /// @return true, the transition always happens in this state
    bool start(WorkItem& item) const override;
    /// @brief escalates the work item: moves it to EscalatedState (the assignee is kept until reassigned)
    /// @param item the work item whose state is changing
    /// @return true, the transition always happens in this state
    bool escalate(WorkItem& item) const override;
    /// @brief cancels the work item: moves it to CancelledState
    /// @param item the work item whose state is changing
    /// @return true, the transition always happens in this state
    bool cancel(WorkItem& item) const override;
    /// @brief gets the name of the state
    /// @return "Assigned"
    std::string name() const override;

private:
    /// @brief The constructor. Private: use instance().
    AssignedState() {}
};

#endif
