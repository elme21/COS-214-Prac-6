// TEMPORARY - stands in for EventPublisher / EventListener (Observer) and TaskRouter /
// AssignmentStrategy (Strategy), Member 6, until those classes exist in include/ and src/.
// This is the ONLY file that wires those two patterns. Delete the stand-in bodies when
// they land; keep the two call sites in EngineGateway.
#ifndef WEB_PENDINGINTEGRATION_H
#define WEB_PENDINGINTEGRATION_H

#include <string>

class WorkItem;
class Participant;
class Trace;

namespace PendingIntegration {

/**
 * Observer hook. EngineGateway calls this after EVERY successful transition.
 *
 * TODO(Member 6): publish an event, e.g.
 *     eventPublisher->publish(WorkflowEvent(instanceId, item->getId(), from, to));
 * and let DependencyActivator (EventListener) call
 *     item->changed("completed");   // -> ResourceCoordinator::notifyChanged
 * AuditLogger can then replace web/server/HistoryLog.
 *
 * Until then this function plays DependencyActivator's part directly: when an
 * item becomes complete it calls the real Colleague::changed(), and the real
 * ResourceCoordinator (Mediator) unlocks the dependants.
 */
void publishTransition(WorkItem* item, const std::string& from, const std::string& to, Trace& trace);

/**
 * Strategy hook. EngineGateway calls this before every assignTo().
 *
 * TODO(Member 6): return taskRouter->route(item, candidates), where TaskRouter holds
 * the AssignmentStrategy set for the task (RoleBasedAssignment, WorkloadAssignment,
 * RuleBasedAssignment), and let the UI's choice become an optional override.
 *
 * Until then the participant picked manually in the UI is returned unchanged.
 */
Participant* chooseParticipant(WorkItem* item, Participant* pickedInUi, Trace& trace);

/** Status lines for the "Generic engine" view. */
std::string observerStatus();
std::string strategyStatus();

}

#endif
