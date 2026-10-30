#ifndef APPROVALREQUEST_H
#define APPROVALREQUEST_H

#include <string>
#include <vector>
#include "WorkComponent.h"

class WorkItem;

/// @brief ApprovalRequest class is an inbetween class that allows work item to interact with the chain of responsibility more cleanly
class ApprovalRequest
{
public:
    /// @brief Outcome enum represents the possible outcomes of an approval request
    enum Outcome { Pending, Approved, Rejected, Unhandled };

    /// @brief The constructor
    /// @param item The work item requesting to enter the chain
    /// @param value The amount requesting approval
    ApprovalRequest(WorkComponent* target, double value = 0.0)
        : target(target), value(value), outcome(Pending) {}

    /// @brief Gets the target work component
    /// @return A pointer to the target work component
    WorkComponent* getTarget() const { return target; }
    
    /// @brief Gets the value of the approval request
    /// @return The value of the approval request
    double getValue() const { return value; }

    /// @brief Adds a handler to the trace
    /// @param handlerName The name of the handler
    void addVisited(const std::string& handlerName) { trace.push_back(handlerName); }
    /// @brief Gets the trace of visited handlers
    /// @return A const reference to the trace vector
    const std::vector<std::string>& getTrace() const { return trace; }

    /// @brief Sets the decision for the approval request
    /// @param result The outcome of the decision
    /// @param handlerName The name of the handler making the decision
    void setDecision(Outcome result, const std::string& handlerName) { outcome = result; decidedBy = handlerName; }

    /// @brief Gets the outcome of the approval request
    /// @return The outcome of the approval request
    Outcome getOutcome() const { return outcome; }

    /// @brief Gets the name of the handler that made the decision
    /// @return A const reference to the handler name
    const std::string& getDecidedBy() const { return decidedBy; }

private:
    /// @brief The target work component requesting approval
    WorkComponent* target;
    /// @brief The value of the approval request
    double value;
    /// @brief The outcome of the approval request
    Outcome outcome;
    /// @brief The name of the handler that made the decision
    std::string decidedBy;
    /// @brief The trace of handlers that have processed the approval request
    std::vector<std::string> trace;
};

#endif