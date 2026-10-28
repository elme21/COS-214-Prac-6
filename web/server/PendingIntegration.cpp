// TEMPORARY - stands in for TaskRouter / AssignmentStrategy (Strategy, Member 6) until they
// compile. Delete the stand-in body when that lands; keep the call site in EngineGateway.
#include "PendingIntegration.h"

#include "Trace.h"

namespace PendingIntegration {

Participant* chooseParticipant(WorkItem*, Participant* pickedInUi, Trace& trace) {
    // TODO(Member 6): taskRouter->setStrategy(...); taskRouter->route(item);
    trace.pending("TaskRouter::route / AssignmentStrategy", "Strategy",
                  "not integrated: the Strategy classes do not compile (route() is void in AssignmentStrategy "
                  "and TaskRouter.h, bool in the .cpp files). The participant was picked manually.");
    return pickedInUi;
}

std::string strategyStatus() {
    return "Not integrated: AssignmentStrategy::route() is declared void but RoleBasedAssignment, "
           "RuleBasedAssignment, WorkloadAssignment and TaskRouter define bool route(), so none of them "
           "compiles. Assignment is manual.";
}

}
