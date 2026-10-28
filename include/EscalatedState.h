#ifndef ESCALATEDSTATE_H
#define ESCALATEDSTATE_H

#include "WorkItemState.h"

/// @brief
/// The EscalatedState class is a ConcreteState of the State pattern: the work item was escalated and
/// waits to be reassigned. Allows assign() -> AssignedState (with a non-null participant) and
/// cancel() -> CancelledState; every other operation is refused.
class EscalatedState : public WorkItemState {
public:
    /// @brief gets the single shared instance of this stateless state
    /// @return the shared EscalatedState instance
    static WorkItemState& instance();

    /// @brief reassigns the escalated work item: sets the new assignee and moves it to AssignedState
    /// @param item the work item whose state is changing
    /// @param participant the participant to assign (non-owning)
    /// @return true if assigned, false if participant is null (nothing changes)
    bool assign(WorkItem& item, Participant* participant) const override;
    /// @brief cancels the work item: moves it to CancelledState
    /// @param item the work item whose state is changing
    /// @return true, the transition always happens in this state
    bool cancel(WorkItem& item) const override;
    /// @brief gets the name of the state
    /// @return "Escalated"
    std::string name() const override;

private:
    /// @brief The constructor. Private: use instance().
    EscalatedState() {}
};

#endif
