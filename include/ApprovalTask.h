#ifndef APPROVALTASK_H
#define APPROVALTASK_H

#include "WorkItem.h"

#include <string>

class ApprovalHandler;

/** A task decided by a chain of approvers; the decision arrives as approve() or reject(). */
class ApprovalTask : public WorkItem {
public:
    ApprovalTask();
    explicit ApprovalTask(ApprovalHandler* firstHandler);
    explicit ApprovalTask(const std::string& name,
                          const std::string& id = std::string(),
                          ApprovalHandler* firstHandler = nullptr,
                          double requestValue = 0.0);

    void setFirstHandler(ApprovalHandler* firstHandler);

    // Value sent down the chain in the ApprovalRequest (e.g. cost or risk score);
    // each handler compares it with its own limit.
    void setRequestValue(double value);
    double getRequestValue() const;

    // Prototype - coordinate with Member 3
    WorkComponent* clone() const override;

protected:
    bool perform() override;

private:
    // NON-OWNING: the approval chain is owned by the workflow definition (Member 4).
    ApprovalHandler* firstHandler;
    double requestValue;
};

#endif
