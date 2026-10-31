#include "ApprovalHandler.h"
#include "ApprovalRequest.h"

void ApprovalHandler::handleRequest(ApprovalRequest& request)
{
    request.addVisited(getName());

    if (tryDecide(request))
    {
        return;
    }
    if (successor != nullptr)
    {
        successor->handleRequest(request);
    }
    else
    {
        request.setDecision(ApprovalRequest::Unhandled, getName());
    }
}