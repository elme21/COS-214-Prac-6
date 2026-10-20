#include "CancelledState.h"

WorkItemState& CancelledState::instance() {
    static CancelledState theInstance;
    return theInstance;
}

std::string CancelledState::name() const { return "Cancelled"; }
