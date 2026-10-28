#ifndef APPROVALTASK_H
#define APPROVALTASK_H

#include "WorkItem.h"

#include <string>

class ApprovalHandler;

/// @brief
/// The ApprovalTask class is a ConcreteClass of the Template Method pattern (and a Composite leaf):
/// a task decided by a chain of approvers; the decision arrives as approve() or reject().
/// It is the Client of the Chain of Responsibility pattern.
class ApprovalTask : public WorkItem {
public:
    /// @brief The default constructor. No approval chain and a request value of 0.
    ApprovalTask();
    /// @brief The constructor (no name or ID, request value 0)
    /// @param firstHandler the first handler of the approval chain (non-owning)
    explicit ApprovalTask(ApprovalHandler* firstHandler);
    /// @brief The constructor
    /// @param name the display name of the task
    /// @param id the ID of the task within its definition
    /// @param firstHandler the first handler of the approval chain (non-owning)
    /// @param requestValue the value sent down the chain (see setRequestValue())
    explicit ApprovalTask(const std::string& name,
                          const std::string& id = std::string(),
                          ApprovalHandler* firstHandler = nullptr,
                          double requestValue = 0.0);

    /// @brief sets the first handler of the approval chain
    /// @param firstHandler the first handler (non-owning)
    void setFirstHandler(ApprovalHandler* firstHandler);

    // Value sent down the chain in the ApprovalRequest (e.g. cost or risk score);
    // each handler compares it with its own limit.
    /// @brief sets the value sent down the chain in the ApprovalRequest (e.g. cost or risk score).
    /// Each handler approves if the value is within its limit, otherwise passes it on, so a higher
    /// value travels further along the chain.
    /// @param value the request value
    void setRequestValue(double value);
    /// @brief gets the value sent down the chain
    /// @return the request value
    double getRequestValue() const;

    // Prototype - coordinate with Member 3
    /// @brief creates a copy of this task (Prototype), including its state, assignee, name, ID, request
    /// value and the same (non-owned) first-handler pointer
    /// @return a new ApprovalTask owned by the caller
    WorkComponent* clone() const override;

protected:
    /// @brief the primitive operation: sends an ApprovalRequest(this, requestValue) to the first handler.
    /// The handler that decides calls approve() or reject() on this task itself. With no chain configured
    /// it prints a message and does nothing.
    /// @return always false, so recordResult() does not complete the task again: the decision has
    /// already been applied by the chain
    bool perform() override;

private:
    // NON-OWNING: the approval chain is owned by the workflow definition (Member 4).
    /// @brief The first handler of the approval chain, or nullptr. NON-OWNING: the approval chain is
    /// owned by the workflow definition (Member 4).
    ApprovalHandler* firstHandler;
    /// @brief The value sent down the chain in the ApprovalRequest; each handler compares it with its limit
    double requestValue;
};

#endif
