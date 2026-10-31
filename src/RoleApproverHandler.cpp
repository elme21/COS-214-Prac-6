#include "RoleApproverHandler.h"
#include "ApprovalRequest.h"
#include "WorkComponent.h"
#include <string>

bool RoleApproverHandler::tryDecide(ApprovalRequest& request)
{
    if (request.getValue() > limit)
    {
        return false;
    }
    request.setDecision(ApprovalRequest::Approved, getName());
    request.getTarget()->approve();
    return true;
}