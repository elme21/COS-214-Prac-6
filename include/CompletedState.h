#ifndef COMPLETEDSTATE_H
#define COMPLETEDSTATE_H

#include "WorkItemState.h"

// Final state: every operation is refused by the WorkItemState base.
/// @brief
/// The CompletedState class is a ConcreteState of the State pattern and a FINAL state:
/// every lifecycle operation is refused (returns false) by the WorkItemState base.
/// It is the only state whose isComplete() returns true, which is what unlocks dependent work items.
class CompletedState : public WorkItemState {
public:
    /// @brief gets the single shared instance of this stateless state
    /// @return the shared CompletedState instance
    static WorkItemState& instance();

    /// @brief reports that the work item is finished, so its dependants may become available
    /// @return true
    bool isComplete() const override { return true; }
    /// @brief gets the name of the state
    /// @return "Completed"
    std::string name() const override;

private:
    /// @brief The constructor. Private: use instance().
    CompletedState() {}
};

#endif
