#ifndef ESCALATEDSTATE_H
#define ESCALATEDSTATE_H

#include "WorkItemState.h"

class EscalatedState : public WorkItemState {
public:
    static WorkItemState& instance();

    bool assign(WorkItem& item, Participant* participant) const override;
    bool cancel(WorkItem& item) const override;
    std::string name() const override;

private:
    EscalatedState() {}
};

#endif
