#include "RoleBasedAssignment.h"
#include "WorkItem.h"      // Person 2: assignTo(Participant*)
#include "Participant.h"   // Person 4: getRole()

RoleBasedAssignment::RoleBasedAssignment(const std::vector<Participant*>& c, const std::string& role)
    : AssignmentStrategy(c), requiredRole(role) {}

void RoleBasedAssignment::route(WorkItem* item) {
    if (!item) return;
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        if (candidates[i] && candidates[i]->getRole() == requiredRole) {
            item->assignTo(candidates[i]);
            return;
        }
    }
}
