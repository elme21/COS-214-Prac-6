#ifndef CREATEDSTATE_H
#define CREATEDSTATE_H

#include "WorkItemState.h"

/// @brief
/// The CreatedState class is a ConcreteState of the State pattern: the initial state of every work item.
/// Allows makeAvailable() -> AvailableState and cancel() -> CancelledState; every other operation is refused.
class CreatedState : public WorkItemState {
public:
    /// @brief gets the single shared instance of this stateless state
    /// @return the shared CreatedState instance
    static WorkItemState& instance();

    /// @brief makes the work item available: moves it to AvailableState
    /// @param item the work item whose state is changing
    /// @return true, the transition always happens in this state
    bool makeAvailable(WorkItem& item) const override;
    /// @brief cancels the work item: moves it to CancelledState
    /// @param item the work item whose state is changing
    /// @return true, the transition always happens in this state
    bool cancel(WorkItem& item) const override;
    /// @brief gets the name of the state
    /// @return "Created"
    std::string name() const override;

private:
    /// @brief The constructor. Private: use instance().
    CreatedState() {}
};

#endif
