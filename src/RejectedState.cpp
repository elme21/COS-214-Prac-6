#include "RejectedState.h"
#include "AssignedState.h"
#include "CancelledState.h"

WorkItemState& RejectedState::instance() {
    static RejectedState theInstance;
    return theInstance;
}

bool RejectedState::assign(WorkItem& item, Participant* participant) const {
    if (participant == nullptr) return false;
    setAssignee(item, participant);
    return transition(item, AssignedState::instance());
}

bool RejectedState::cancel(WorkItem& item) const { return transition(item, CancelledState::instance()); }

std::string RejectedState::name() const { return "Rejected"; }
