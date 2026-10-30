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
    /// @param role the role that the handler is responsible for approving
    /// @param limit the maximum amount this handler can approve
    /// @param next the next handler in the chain
    RoleApproverHandler(const std::string& name, const std::string& role, double limit, ApprovalHandler* next = nullptr)
        : ApprovalHandler(name, next), role(role), limit(limit) {}

    /// @brief Handles the approval request. If the request's role matches this handler's role and the value is within the limit, it approves the request. Otherwise, it passes the request to the next handler in the chain.
    /// @param request the approval request to handle
    void handleRequest(ApprovalRequest& request);

private:
    /// @brief Checks if the request is handlable based on the role and limit.
    /// @param request the approval request to check
    /// @return true if the request is handlable, false otherwise
    bool isAuthorized(const ApprovalRequest& request) const;
    /// @brief The name of the handler
    std::string name;
    /// @brief The role that this handler is responsible for approving
    std::string role;
    /// @brief The maximum amount this handler can approve
    double limit;
    bool tryDecide(ApprovalRequest &request);
    
};

#endif