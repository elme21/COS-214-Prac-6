#ifndef APPROVALTASK_H
#define APPROVALTASK_H

#include "WorkItem.h"

class ApprovalHandler;

/** A task decided by a chain of approvers; the decision arrives as approve() or reject(). */
class ApprovalTask : public WorkItem {
public:
    ApprovalTask();
    explicit ApprovalTask(ApprovalHandler* firstHandler);

    void setFirstHandler(ApprovalHandler* firstHandler);

    // Prototype - coordinate with Member 3
    WorkComponent* clone() const override;

protected:
    bool perform() override;

private:
    // NON-OWNING: the approval chain is owned by the workflow definition (Member 4).
    ApprovalHandler* firstHandler;
};

#endif
