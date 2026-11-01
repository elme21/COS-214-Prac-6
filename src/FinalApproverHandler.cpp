#include "FinalApproverHandler.h"
#include "ApprovalRequest.h"
#include "WorkComponent.h"

#include <string>

bool FinalApproverHandler::tryDecide(ApprovalRequest& request)
{
    if (request.getValue() <= limit)
    {
        request.setDecision(ApprovalRequest::Approved, getName());
        request.getTarget()->approve();
    }
    else
    {
        request.setDecision(ApprovalRequest::Rejected, getName());
        request.getTarget()->reject();
    }
    return true;
}