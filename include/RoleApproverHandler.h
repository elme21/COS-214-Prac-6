#ifndef ROLE_APPROVER_HANDLER_H
#define ROLE_APPROVER_HANDLER_H

#include <string>
#include "ApprovalHandler.h"

class RoleApproverHandler : public ApprovalHandler
{
public:
    RoleApproverHandler(const std::string& role, double limit, ApprovalHandler* next = nullptr)
        : ApprovalHandler(next), role(role), limit(limit) {}

    void handleRequest(ApprovalRequest& request) override;

private:
    bool isAuthorized(const ApprovalRequest& request) const;

    std::string role;
    double limit;
};

#endif