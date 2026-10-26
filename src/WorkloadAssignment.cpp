#include "WorkloadAssignment.h"
#include "WorkItem.h"
#include "Participant.h"   // Person 4: workload()

WorkloadAssignment::WorkloadAssignment(const std::vector<Participant*>& c) : AssignmentStrategy(c) {}

void WorkloadAssignment::route(WorkItem* item) {
    if (!item) return;
    Participant* best = nullptr;
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        if (!candidates[i]) continue;
        if (best == nullptr || candidates[i]->workload() < best->workload()) best = candidates[i];
    }
    if (best) item->assignTo(best);
}
