// TEMPORARY - stands in for EventPublisher / EventListener (Observer) and TaskRouter /
// AssignmentStrategy (Strategy), Member 6, until those classes exist in include/ and src/.
// This is the ONLY file that wires those two patterns. Delete the stand-in bodies when
// they land; keep the two call sites in EngineGateway.
#include "PendingIntegration.h"

#include "Trace.h"
#include "WorkItem.h"

// TODO(Member 6): #include "EventPublisher.h", "DependencyActivator.h", "AuditLogger.h",
//                 "TaskRouter.h", "AssignmentStrategy.h" once they exist.

namespace PendingIntegration {

void publishTransition(WorkItem* item, const std::string& from, const std::string& to, Trace& trace) {
    // TODO(Member 6): eventPublisher->publish(...) replaces everything below.
    trace.pending("EventPublisher::publish(" + from + " -> " + to + ")", "Observer",
                  "not yet integrated (Member 6): no event is published, no listener runs");

    if (to != "Completed") return;

    // Stand-in for DependencyActivator::update(): tell the Mediator this colleague changed.
    trace.temporary("DependencyActivator stand-in (PendingIntegration.cpp)", "Observer",
                    "calls the real Colleague::changed(\"completed\") on " + item->getName());
    item->changed("completed");
    trace.engine("Colleague::changed -> ResourceCoordinator::notifyChanged", "Mediator",
                 "each dependant's WorkItem::receive() asks canBecomeAvailable() and, if every "
                 "prerequisite is complete, calls makeAvailable()");
}

Participant* chooseParticipant(WorkItem*, Participant* pickedInUi, Trace& trace) {
    // TODO(Member 6): return taskRouter->route(item, candidates);
    trace.pending("TaskRouter::route / AssignmentStrategy", "Strategy",
                  "not yet integrated (Member 6): the participant was picked manually in the UI");
    return pickedInUi;
}

std::string observerStatus() {
    return "Not yet integrated. PendingIntegration::publishTransition stands in for DependencyActivator by "
           "calling Colleague::changed(); HistoryLog stands in for AuditLogger.";
}

std::string strategyStatus() {
    return "Not yet integrated. PendingIntegration::chooseParticipant returns the participant picked in the UI.";
}

}
