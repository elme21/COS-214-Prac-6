#ifndef ROLE_APPROVER_HANDLER_H
#define ROLE_APPROVER_HANDLER_H

#include <string>
#include "ApprovalHandler.h"

/// @brief The RoleApproverHandler class is a ConcreteHandler
class RoleApproverHandler : public ApprovalHandler
{
public:
    /// @brief The constructor for the RoleApproverHandler class.
    /// @param name the name of the handler
    /// @param limit the maximum amount this handler can approve
    /// @param next the next handler in the chain
    RoleApproverHandler(const std::string& name, double limit, ApprovalHandler* next = nullptr)
    : ApprovalHandler(name, next), limit(limit) {}

    /// @brief Handles the approval request. If the request's role matches this handler's role and the value is within the limit, it approves the request. Otherwise, it passes the request to the next handler in the chain.
    /// @param request the approval request to handle
    void handleRequest(ApprovalRequest& request);

protected:
    /// @brief Tries to decide on an approval request. If the request's role matches this handler's role and the value is within the limit, it approves the request. Otherwise, it returns false to indicate that it cannot handle the request.
    /// @param request the approval request to decide on
    /// @return true if this handler decided the request, false otherwise
    bool tryDecide(ApprovalRequest& request) override;

private:
    /// @brief Checks if the request is handlable based on the role and limit.
    /// @param request the approval request to check
    /// @return true if the request is handlable, false otherwise
    bool isAuthorized(const ApprovalRequest& request) const;
    /// @brief The role that this handler is responsible for approving
    std::string role;
    /// @brief The maximum amount this handler can approve
    double limit;
};

#endif