#include "ApprovalTask.h"
#include "ApprovalHandler.h"

#include <iostream>

ApprovalTask::ApprovalTask() : firstHandler(nullptr) {}

ApprovalTask::ApprovalTask(ApprovalHandler* handler) : firstHandler(handler) {}

void ApprovalTask::setFirstHandler(ApprovalHandler* handler) { firstHandler = handler; }

WorkComponent* ApprovalTask::clone() const { return new ApprovalTask(*this); }

bool ApprovalTask::perform() {
    if (firstHandler == nullptr) {
        std::cout << "[ApprovalTask] no approval chain configured" << std::endl;
        return false;
    }
    std::cout << "[ApprovalTask] sending request to first handler" << std::endl;
    // TODO align ApprovalRequest with Member 4
    ApprovalRequest request(this);
    firstHandler->handle(request);
    return false;  // the decision comes back later as approve() or reject()
}
