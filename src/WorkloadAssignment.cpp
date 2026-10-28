/**
 * @file WorkloadAssignment.cpp
 * @brief Implementation of WorkloadAssignment.
 *
 * @details A single pass finds the candidate with the lowest workload; the strict less-than comparison makes the earliest candidate win ties.
 *
 * @author Logan (Observer and Strategy)
 * @date 2026-10-28
 * @ingroup StrategyPattern
 */
#include "WorkloadAssignment.h"
#include "WorkItem.h"
#include "Participant.h"

WorkloadAssignment::WorkloadAssignment(const std::vector<Participant*>& c) : AssignmentStrategy(c) {}

bool WorkloadAssignment::route(WorkItem* item) {
    if (!item) return false;
    Participant* best = nullptr;
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        if (!candidates[i]) continue;
        if (best == nullptr || candidates[i]->workload() < best->workload()) best = candidates[i];
    }
    return best != nullptr && item->assignTo(best);
}
