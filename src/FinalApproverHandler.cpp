#include "FinalApproverHandler.h"
#include "ApprovalRequest.h"
#include "WorkItem.h"

void FinalApproverHandler::handleRequest(ApprovalRequest& request)
{
    if (request.getValue() <= limit)
    {
        request.getItem()->approve();
    }
    else
    {
        request.getItem()->reject();
    }
}