// TEMPORARY - stands in for TaskRouter / AssignmentStrategy (Strategy, Member 6) until they
// compile: AssignmentStrategy::route() is declared void, but RoleBasedAssignment,
// RuleBasedAssignment, WorkloadAssignment and TaskRouter define bool route(). Delete the
// stand-in body when that lands; keep the call site in EngineGateway.
#ifndef WEB_PENDINGINTEGRATION_H
#define WEB_PENDINGINTEGRATION_H

#include <string>

class WorkItem;
class Participant;
class Trace;

namespace PendingIntegration {

/**
 * Strategy hook. EngineGateway calls this before every manual assignment.
 *
 * TODO(Member 6): once the Strategy classes compile, offer "auto-assign": call
 * taskRouter->setStrategy(strategyForTask) then taskRouter->route(item), and show which
 * strategy chose the participant. The manual pick stays as an override.
 *
 * Until then the participant picked manually in the UI is returned unchanged.
 */
Participant* chooseParticipant(WorkItem* item, Participant* pickedInUi, Trace& trace);

/** Status line for the "Generic engine" view. */
std::string strategyStatus();

}

#endif
