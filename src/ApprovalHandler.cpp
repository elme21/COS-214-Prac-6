#include "ApprovalHandler.h"

void ApprovalHandler::handleRequest(ApprovalRequest& request)
{
    if (successor != nullptr)
    {
        successor->handleRequest(request);
    }
}