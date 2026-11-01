#include "ApprovalTask.h"
#include "ApprovalHandler.h"
#include "ApprovalRequest.h"

#include <iostream>

namespace {
const char* const kNoDecision = "no decision yet";
}

ApprovalTask::ApprovalTask()
    : firstHandler(nullptr), requestValue(0.0), chainRan(false), chainDecided(false), chainApproved(false),
      chainOutcome(kNoDecision) {}

ApprovalTask::ApprovalTask(ApprovalHandler* handler)
    : firstHandler(handler), requestValue(0.0), chainRan(false), chainDecided(false), chainApproved(false),
      chainOutcome(kNoDecision) {}

ApprovalTask::ApprovalTask(const std::string& name, const std::string& id,
                           ApprovalHandler* handler, double value)
    : WorkItem(name, id), firstHandler(handler), requestValue(value), chainRan(false), chainDecided(false),
      chainApproved(false), chainOutcome(kNoDecision) {}

void ApprovalTask::setFirstHandler(ApprovalHandler* handler) { firstHandler = handler; }

void ApprovalTask::setRequestValue(double value) { requestValue = value; }

double ApprovalTask::getRequestValue() const { return requestValue; }

bool ApprovalTask::hasChainResult() const { return chainRan; }

const std::string& ApprovalTask::getChainOutcome() const { return chainOutcome; }

bool ApprovalTask::wasChainDecided() const { return chainDecided; }

bool ApprovalTask::wasChainApproved() const { return chainApproved; }

const std::vector<std::string>& ApprovalTask::getChainPath() const { return chainPath; }

const std::string& ApprovalTask::getChainDecidedBy() const { return chainDecidedBy; }

WorkComponent* ApprovalTask::clone() const { return new ApprovalTask(*this); }

bool ApprovalTask::perform()
{
    clearChainResult();
    if (firstHandler == nullptr)
    {
        std::cout << "[ApprovalTask] no approval chain configured" << std::endl;
        return false;
    }

    ApprovalRequest request(this, requestValue);
    firstHandler->handleRequest(request);
    keepChainResult(request);
    return false;
}

void ApprovalTask::clearChainResult()
{
    chainRan = false;
    chainDecided = false;
    chainApproved = false;
    chainOutcome = kNoDecision;
    chainDecidedBy.clear();
    chainPath.clear();
}

void ApprovalTask::keepChainResult(const ApprovalRequest& request)
{
    // Indexed by ApprovalRequest::Outcome { Pending, Approved, Rejected, Unhandled }.
    static const char* const outcomeNames[] = { "pending", "approved", "rejected", "unhandled" };

    chainRan = true;
    chainApproved = request.getOutcome() == ApprovalRequest::Approved;
    chainDecided = chainApproved || request.getOutcome() == ApprovalRequest::Rejected;
    chainOutcome = outcomeNames[request.getOutcome()];
    chainPath = request.getTrace();
    chainDecidedBy = chainDecided ? request.getDecidedBy() : std::string();
    // The handler that stops the chain is the last one visited; use it if the handler left decidedBy empty.
    if (chainDecided && chainDecidedBy.empty() && !chainPath.empty()) chainDecidedBy = chainPath.back();
}
