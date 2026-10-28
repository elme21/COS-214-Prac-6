#ifndef FINAL_APPROVER_HANDLER_H
#define FINAL_APPROVER_HANDLER_H

#include "ApprovalHandler.h"

/// @brief The FinalApproverHandler class is a ConcreteHandler that approves requests up to a certain limit. If the request exceeds the limit, it is denied.
class FinalApproverHandler : public ApprovalHandler
{
public:
    /// @brief Constructs a FinalApproverHandler with a specified limit.
    /// @param name The name of the handler.
    /// @param limit The maximum amount this handler can approve.
   FinalApproverHandler(const std::string& name, double limit)
    : ApprovalHandler(name, nullptr), limit(limit) {}

    /// @brief Handles the approval request. If the request's value is within the limit, it approves the request. Otherwise, it denies the request.
    /// @param request The approval request to handle.
    void handleRequest(ApprovalRequest& request);

protected:
    /// @brief Tries to decide on an approval request. If the request's value is within the limit, it approves the request. Otherwise, it returns false to indicate that it cannot handle the request.
    /// @param request The approval request to decide on.
    /// @return True if this handler decided the request, false otherwise.
    bool tryDecide(ApprovalRequest& request) override;

private:
    /// @brief The name of the handler.
    std::string name;
    /// @brief The maximum amount this handler can approve.
    double limit;
};

#endif