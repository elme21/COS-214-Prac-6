#ifndef APPROVALREQUEST_H
#define APPROVALREQUEST_H

#include <string>
#include <vector>

class WorkComponent;

/// @brief The ApprovalRequest class represents a request for approval of a work component. It contains information about the target work component, the value of the request, and the outcome of the approval process.
class ApprovalRequest
{
public:
    /// @brief The Outcome enum represents the possible outcomes of an approval request.
    enum Outcome { Pending, Approved, Rejected, Unhandled };

    /// @brief Constructs an approval request with a target work component and a value.
    /// @param target The work component that is the subject of the approval request
    /// @param value The value associated with the approval request
    ApprovalRequest(WorkComponent* target, double value = 0.0)
        : target(target), value(value), outcome(Pending) {}

    /// @brief Gets the target work component of the approval request.
    /// @return A pointer to the target work component.
    WorkComponent* getTarget() const { return target; }
    /// @brief Gets the value associated with the approval request.
    /// @return The value of the approval request.
    double getValue() const { return value; }

    /// @brief Adds a handler name to the trace of visited handlers.
    /// @param handlerName The name of the handler that processed the request.
    void addVisited(const std::string& handlerName) { trace.push_back(handlerName); }
    /// @brief Gets the trace of visited handlers for this approval request.
    /// @return A const reference to a vector of handler names that have processed the request.
    const std::vector<std::string>& getTrace() const { return trace; }

    /// @brief Sets the decision for the approval request.
    /// @param result The outcome of the decision.
    /// @param handlerName The name of the handler that made the decision.
    void setDecision(Outcome result, const std::string& handlerName) { outcome = result; decidedBy = handlerName; }
    /// @brief Gets the outcome of the approval request.
    /// @return The outcome of the approval request.
    Outcome getOutcome() const { return outcome; }
    /// @brief Gets the name of the handler that made the decision for the approval request.
    /// @return The name of the handler that made the decision.
    const std::string& getDecidedBy() const { return decidedBy; }

private:
    WorkComponent* target;
    double value;
    Outcome outcome;
    std::string decidedBy;
    std::vector<std::string> trace;
};

#endif