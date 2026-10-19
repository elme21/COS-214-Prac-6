#ifndef INPROGRESSSTATE_H
#define INPROGRESSSTATE_H

#include "WorkItemState.h"

class InProgressState : public WorkItemState {
public:
    static WorkItemState& instance();

    bool complete(WorkItem& item) const override;
    bool approve(WorkItem& item) const override;
    bool reject(WorkItem& item) const override;
    bool escalate(WorkItem& item) const override;
    bool cancel(WorkItem& item) const override;
    bool canExecute() const override;
    std::string name() const override;

private:
    InProgressState() {}
};

#endif
