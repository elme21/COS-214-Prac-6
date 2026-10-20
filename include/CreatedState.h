#ifndef CREATEDSTATE_H
#define CREATEDSTATE_H

#include "WorkItemState.h"

class CreatedState : public WorkItemState {
public:
    static WorkItemState& instance();

    bool makeAvailable(WorkItem& item) const override;
    bool cancel(WorkItem& item) const override;
    std::string name() const override;

private:
    CreatedState() {}
};

#endif
