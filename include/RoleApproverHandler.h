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
    /// @brief Gets the role this handler represents
    const std::string& getRole() const { return role; }
    /// @brief Gets the highest value this handler may approve
    double getLimit() const { return limit; }
private:
    /// @brief The role that this handler is responsible for approving
    std::string role;
    /// @brief The maximum amount this handler can approve
    double limit;
protected:
    /// @brief Approves the request if its value is within this handler's limit, otherwise passes it on.
    /// @param request The approval request to decide on.
    /// @return true if this handler decided the request.
    bool tryDecide(ApprovalRequest &request) override;
};

#endif