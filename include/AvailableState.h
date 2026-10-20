#ifndef AVAILABLESTATE_H
#define AVAILABLESTATE_H

#include "WorkItemState.h"

class AvailableState : public WorkItemState {
public:
    static WorkItemState& instance();

    bool assign(WorkItem& item, Participant* participant) const override;
    bool cancel(WorkItem& item) const override;
    std::string name() const override;

private:
    AvailableState() {}
};

#endif
