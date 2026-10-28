#ifndef FINAL_APPROVER_HANDLER_H
#define FINAL_APPROVER_HANDLER_H

#include "ApprovalHandler.h"

/// @brief The FinalApproverHandler class is a ConcreteHandler that approves requests up to a certain limit. If the request exceeds the limit, it is denied.
class FinalApproverHandler : public ApprovalHandler
{
public:
    /// @brief Constructs a FinalApproverHandler with a specified limit.
    /// @param limit The maximum amount this handler can approve.
    explicit FinalApproverHandler(double limit) : ApprovalHandler(nullptr), limit(limit) {}

    /// @brief Handles the approval request. If the request's value is within the limit, it approves the request. Otherwise, it denies the request.
    /// @param request The approval request to handle.
    void handleRequest(ApprovalRequest& request) override;

private:
    /// @brief The maximum amount this handler can approve.
    double limit;
};

#endif