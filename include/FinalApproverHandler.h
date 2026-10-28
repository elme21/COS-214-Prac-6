#ifndef FINAL_APPROVER_HANDLER_H
#define FINAL_APPROVER_HANDLER_H

#include "ApprovalHandler.h"

/// @brief The FinalApproverHandler class is a ConcreteHandler that approves requests up to a certain limit. If the request exceeds the limit, it is denied.
class FinalApproverHandler : public ApprovalHandler
{
public:
    /// @brief Constructs a FinalApproverHandler with a specified limit.
    /// @param limit The maximum amount this handler can approve.
    explicit FinalApproverHandler(std::string& name, double limit) : ApprovalHandler(name, nullptr), limit(limit) {}

    /// @brief Tries to decide on the approval request.
    /// @param request The approval request to decide on.
    /// @return true if the request was handled, false otherwise.
    bool FinalApproverHandler::tryDecide(ApprovalRequest& request);
    
    /// @brief Handles the approval request. If the request's value is within the limit, it approves the request. Otherwise, it denies the request.
    /// @param request The approval request to handle.
    void handleRequest(ApprovalRequest& request);

private:
    /// @brief The name of the handler
    std::string name;
    /// @brief The maximum amount this handler can approve.
    double limit;
};

#endif