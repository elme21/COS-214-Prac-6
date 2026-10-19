#include "WorkItemState.h"
#include "WorkItem.h"

bool WorkItemState::makeAvailable(WorkItem&) const { return false; }
bool WorkItemState::assign(WorkItem&, Participant*) const { return false; }
bool WorkItemState::start(WorkItem&) const { return false; }
bool WorkItemState::approve(WorkItem&) const { return false; }
bool WorkItemState::complete(WorkItem&) const { return false; }
bool WorkItemState::reject(WorkItem&) const { return false; }
bool WorkItemState::cancel(WorkItem&) const { return false; }
bool WorkItemState::escalate(WorkItem&) const { return false; }
bool WorkItemState::canExecute() const { return false; }

bool WorkItemState::transition(WorkItem& item, const WorkItemState& next) {
    item.changeState(&next);
    return true;
}

void WorkItemState::setAssignee(WorkItem& item, Participant* participant) {
    item.setAssignee(participant);
}
