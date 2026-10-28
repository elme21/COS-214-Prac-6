#ifndef INPROGRESSSTATE_H
#define INPROGRESSSTATE_H

#include "WorkItemState.h"

/// @brief
/// The InProgressState class is a ConcreteState of the State pattern: the assignee is working on the item.
/// Allows complete() -> CompletedState, approve() -> CompletedState, reject() -> RejectedState,
/// escalate() -> EscalatedState and cancel() -> CancelledState; makeAvailable(), assign() and start()
/// are refused. It is the only state whose canExecute() returns true.
class InProgressState : public WorkItemState {
public:
    /// @brief gets the single shared instance of this stateless state
    /// @return the shared InProgressState instance
    static WorkItemState& instance();

    /// @brief completes the work item: moves it to CompletedState
    /// @param item the work item whose state is changing
    /// @return true, the transition always happens in this state
    bool complete(WorkItem& item) const override;
    /// @brief approves the work item: moves it to CompletedState (approval finishes the item)
    /// @param item the work item whose state is changing
    /// @return true, the transition always happens in this state
    bool approve(WorkItem& item) const override;
    /// @brief rejects the work item: moves it to RejectedState
    /// @param item the work item whose state is changing
    /// @return true, the transition always happens in this state
    bool reject(WorkItem& item) const override;
    /// @brief escalates the work item: moves it to EscalatedState
    /// @param item the work item whose state is changing
    /// @return true, the transition always happens in this state
    bool escalate(WorkItem& item) const override;
    /// @brief cancels the work item: moves it to CancelledState
    /// @param item the work item whose state is changing
    /// @return true, the transition always happens in this state
    bool cancel(WorkItem& item) const override;
    /// @brief allows the template method WorkItem::execute() to run perform()
    /// @return true
    bool canExecute() const override;
    /// @brief gets the name of the state
    /// @return "InProgress"
    std::string name() const override;

private:
    /// @brief The constructor. Private: use instance().
    InProgressState() {}
};

#endif
