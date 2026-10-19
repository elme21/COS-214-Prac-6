#ifndef CANCELLEDSTATE_H
#define CANCELLEDSTATE_H

#include "WorkItemState.h"

// Final state: every operation is refused by the WorkItemState base.
class CancelledState : public WorkItemState {
public:
    static WorkItemState& instance();

    std::string name() const override;

private:
    CancelledState() {}
};

#endif
