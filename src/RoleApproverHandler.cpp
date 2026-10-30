#include "RoleApproverHandler.h"
#include "ApprovalRequest.h"
#include <string>

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