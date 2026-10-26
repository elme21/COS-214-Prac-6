#include "CreatedState.h"
#include "AvailableState.h"
#include "CancelledState.h"

WorkItemState& CreatedState::instance() {
    static CreatedState theInstance;
    return theInstance;
}

bool CreatedState::makeAvailable(WorkItem& item) const { return transition(item, AvailableState::instance()); }

bool CreatedState::cancel(WorkItem& item) const { return transition(item, CancelledState::instance()); }

std::string CreatedState::name() const { return "Created"; }
