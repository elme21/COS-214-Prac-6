#include "FinalApproverHandler.h"
#include "ApprovalRequest.h"
#include "WorkItem.h"

#include <string>

bool FinalApproverHandler::tryDecide(ApprovalRequest& request)
{
    if (request.getValue() <= limit)
    {
        request.setDecision(ApprovalRequest::Approved, name);
        request.getTarget()->approve();
    }
    else
    {
        request.setDecision(ApprovalRequest::Rejected, name);
        request.getTarget()->reject();
    }
    return true;
}