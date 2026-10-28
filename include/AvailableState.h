#ifndef AVAILABLESTATE_H
#define AVAILABLESTATE_H

#include "WorkItemState.h"

/// @brief
/// The AvailableState class is a ConcreteState of the State pattern: the work item is ready to be picked up.
/// Allows assign() -> AssignedState (with a non-null participant) and cancel() -> CancelledState;
/// every other operation is refused.
class AvailableState : public WorkItemState {
public:
    /// @brief gets the single shared instance of this stateless state
    /// @return the shared AvailableState instance
    static WorkItemState& instance();

    /// @brief assigns the work item: sets the assignee and moves it to AssignedState
    /// @param item the work item whose state is changing
    /// @param participant the participant to assign (non-owning)
    /// @return true if assigned, false if participant is null (nothing changes)
    bool assign(WorkItem& item, Participant* participant) const override;
    /// @brief cancels the work item: moves it to CancelledState
    /// @param item the work item whose state is changing
    /// @return true, the transition always happens in this state
    bool cancel(WorkItem& item) const override;
    /// @brief gets the name of the state
    /// @return "Available"
    std::string name() const override;

private:
    /// @brief The constructor. Private: use instance().
    AvailableState() {}
};

#endif
