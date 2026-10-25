#include "RoleApproverHandler.h"
#include "ApprovalRequest.h"
#include "WorkItem.h"

void RoleApproverHandler::handleRequest(ApprovalRequest& request)
{
    if (isAuthorized(request))
    {
        request.getItem()->approve();
    }
    else
    {
        ApprovalHandler::handleRequest(request);   // escalate
    }
}

bool RoleApproverHandler::isAuthorized(const ApprovalRequest& request) const
{
    return request.getValue() <= limit;
}