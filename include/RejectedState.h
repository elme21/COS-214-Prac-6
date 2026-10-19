#ifndef REJECTEDSTATE_H
#define REJECTEDSTATE_H

#include "WorkItemState.h"

class RejectedState : public WorkItemState {
public:
    static WorkItemState& instance();

    bool assign(WorkItem& item, Participant* participant) const override;
    bool cancel(WorkItem& item) const override;
    std::string name() const override;

private:
    RejectedState() {}
};

#endif
