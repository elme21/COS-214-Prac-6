#include "RoleApproverHandler.h"
#include "ApprovalRequest.h"
#include <string>

RoleApproverHandler::RoleApproverHandler(const std::string& name, const std::string& role, double limit, ApprovalHandler* next)
    : ApprovalHandler(name, next), role(role), limit(limit) {}

bool RoleApproverHandler::tryDecide(ApprovalRequest& request)
{
    if (request.getValue() > limit)
    {
        return false;
    }
    request.setDecision(ApprovalRequest::Approved, name);
    request.getTarget()->approve();
    return true;
}