#include "ApprovalTask.h"
#include "ApprovalHandler.h"
#include "ApprovalRequest.h"

#include <iostream>

ApprovalTask::ApprovalTask() : firstHandler(nullptr) {}

ApprovalTask::ApprovalTask(ApprovalHandler* handler) : firstHandler(handler) {}

void ApprovalTask::setFirstHandler(ApprovalHandler* handler) { firstHandler = handler; }

WorkComponent* ApprovalTask::clone() const { return new ApprovalTask(*this); }

bool ApprovalTask::perform()
{
    if (firstHandler == nullptr)
    {
        std::cout << "[ApprovalTask] no approval chain configured" << std::endl;
        return false;
    }

    ApprovalRequest request(this);
    firstHandler->handleRequest(request);
    return false;
}
