#include "ApprovalTask.h"
#include "ApprovalHandler.h"
#include "ApprovalRequest.h"

#include <iostream>

ApprovalTask::ApprovalTask() : firstHandler(nullptr), requestValue(0.0) {}

ApprovalTask::ApprovalTask(ApprovalHandler* handler) : firstHandler(handler), requestValue(0.0) {}

ApprovalTask::ApprovalTask(const std::string& name, const std::string& id,
                           ApprovalHandler* handler, double value)
    : WorkItem(name, id), firstHandler(handler), requestValue(value) {}

void ApprovalTask::setFirstHandler(ApprovalHandler* handler) { firstHandler = handler; }

void ApprovalTask::setRequestValue(double value) { requestValue = value; }

double ApprovalTask::getRequestValue() const { return requestValue; }

WorkComponent* ApprovalTask::clone() const { return new ApprovalTask(*this); }

bool ApprovalTask::perform()
{
    if (firstHandler == nullptr)
    {
        std::cout << "[ApprovalTask] no approval chain configured" << std::endl;
        return false;
    }

    ApprovalRequest request(this, requestValue);
    firstHandler->handleRequest(request);
    return false;
}
