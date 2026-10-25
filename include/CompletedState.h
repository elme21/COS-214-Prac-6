#ifndef COMPLETEDSTATE_H
#define COMPLETEDSTATE_H

#include "WorkItemState.h"

// Final state: every operation is refused by the WorkItemState base.
class CompletedState : public WorkItemState {
public:
    static WorkItemState& instance();

    bool isComplete() const override { return true; }
    std::string name() const override;

private:
    CompletedState() {}
};

#endif
