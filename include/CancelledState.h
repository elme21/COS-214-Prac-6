#ifndef CANCELLEDSTATE_H
#define CANCELLEDSTATE_H

#include "WorkItemState.h"

// Final state: every operation is refused by the WorkItemState base.
/// @brief
/// The CancelledState class is a ConcreteState of the State pattern and a FINAL state:
/// every lifecycle operation is refused (returns false) by the WorkItemState base,
/// and isComplete() stays false, so a cancelled item never unlocks its dependants.
class CancelledState : public WorkItemState {
public:
    /// @brief gets the single shared instance of this stateless state
    /// @return the shared CancelledState instance
    static WorkItemState& instance();

    /// @brief gets the name of the state
    /// @return "Cancelled"
    std::string name() const override;

private:
    /// @brief The constructor. Private: use instance().
    CancelledState() {}
};

#endif
