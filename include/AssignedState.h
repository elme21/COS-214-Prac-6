#ifndef ASSIGNEDSTATE_H
#define ASSIGNEDSTATE_H

#include "WorkItemState.h"

class AssignedState : public WorkItemState {
public:
    static WorkItemState& instance();

    bool start(WorkItem& item) const override;
    bool escalate(WorkItem& item) const override;
    bool cancel(WorkItem& item) const override;
    std::string name() const override;

private:
    AssignedState() {}
};

#endif
