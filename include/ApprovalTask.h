#ifndef APPROVALTASK_H
#define APPROVALTASK_H

#include "WorkItem.h"

#include <string>
#include <vector>

class ApprovalHandler;
class ApprovalRequest;

/// @brief
/// The ApprovalTask class is a ConcreteClass of the Template Method pattern (and a Composite leaf):
/// a task decided by a chain of approvers; the decision arrives as approve() or reject().
/// It is the Client of the Chain of Responsibility pattern.
/// After each run of the chain it keeps the result as plain data (the handlers the request visited,
/// who decided and what was decided), readable through the getChain...() getters. The result holds
/// no pointers, so clone() simply copies it and a clone never refers back to the original task.
class ApprovalTask : public WorkItem {
public:
    /// @brief The default constructor. No approval chain, a request value of 0 and no chain result yet.
    ApprovalTask();
    /// @brief The constructor (no name or ID, request value 0, no chain result yet)
    /// @param firstHandler the first handler of the approval chain (non-owning)
    explicit ApprovalTask(ApprovalHandler* firstHandler);
    /// @brief The constructor (no chain result yet)
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

    /// @brief checks whether the chain has run for this task (perform() sent a request to a configured chain)
    /// @return true once the chain has handled a request, false if it never ran
    bool hasChainResult() const;
    /// @brief gets the outcome of the last chain run, as text
    /// @return "no decision yet" if the chain never ran; otherwise "approved", "rejected",
    /// "unhandled" (the request reached the end of the chain and nobody decided) or "pending"
    const std::string& getChainOutcome() const;
    /// @brief checks whether a handler made a decision (approved or rejected) in the last chain run
    /// @return true if the last run was approved or rejected, false if it never ran or nobody decided
    bool wasChainDecided() const;
    /// @brief checks whether the last chain run approved the request
    /// @return true only if the last run ended in an approval
    bool wasChainApproved() const;
    /// @brief gets the names of the handlers the last request visited, in order (ApprovalRequest::getTrace())
    /// @return the visited handler names; empty if the chain never ran
    const std::vector<std::string>& getChainPath() const;
    /// @brief gets the name of the handler that decided the last request
    /// @return ApprovalRequest::getDecidedBy(), or, if a handler left that empty, the last handler the
    /// request visited (the handler that stops the chain is always the last one visited). Empty if
    /// nobody decided; check wasChainDecided() first.
    const std::string& getChainDecidedBy() const;

    // Prototype - coordinate with Member 3
    /// @brief creates a copy of this task (Prototype), including its state, assignee, name, ID, request
    /// value, the same (non-owned) first-handler pointer and an independent copy of the last chain result
    /// @return a new ApprovalTask owned by the caller
    WorkComponent* clone() const override;

protected:
    /// @brief the primitive operation: clears the previous chain result, then sends an
    /// ApprovalRequest(this, requestValue) to the first handler. The handler that decides calls
    /// approve() or reject() on this task itself. Afterwards the request's path, outcome and decider are
    /// kept as the chain result. With no chain configured it prints a message and leaves the result
    /// as "no decision yet".
    /// @return always false, so recordResult() does not complete the task again: the decision has
    /// already been applied by the chain
    bool perform() override;

private:
    /// @brief resets the chain result to "no decision yet"
    void clearChainResult();
    /// @brief copies what the chain recorded in the request into plain data members
    /// @param request the request the chain has just handled
    void keepChainResult(const ApprovalRequest& request);

    // NON-OWNING: the approval chain is owned by the workflow definition (Member 4).
    /// @brief The first handler of the approval chain, or nullptr. NON-OWNING: the approval chain is
    /// owned by the workflow definition (Member 4).
    ApprovalHandler* firstHandler;
    /// @brief The value sent down the chain in the ApprovalRequest; each handler compares it with its limit
    double requestValue;
    /// @brief Whether the chain has handled a request for this task
    bool chainRan;
    /// @brief Whether a handler approved or rejected the last request
    bool chainDecided;
    /// @brief Whether the last request was approved
    bool chainApproved;
    /// @brief Outcome of the last chain run as text ("no decision yet" until the chain runs)
    std::string chainOutcome;
    /// @brief Name of the handler that decided the last request; empty if nobody decided
    std::string chainDecidedBy;
    /// @brief Names of the handlers the last request visited, in order
    std::vector<std::string> chainPath;
};

#endif
