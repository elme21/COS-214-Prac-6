#include "RoleApproverHandler.h"
#include "ApprovalRequest.h"
#include "WorkItem.h"

bool RoleApproverHandler::tryDecide(ApprovalRequest& request)
{
    if (request.getValue() > limit)
    {
        return false;   // beyond my authority: pass it up
    }
    request.setDecision(ApprovalRequest::Approved, getName());
    request.getTarget()->approve();
    return true;
}