/**
 * @file RoleBasedAssignment.cpp
 * @brief Implementation of RoleBasedAssignment.
 *
 * @details Scans the candidates in order and assigns to the first one whose role equals the required role.
 *
 * @author Logan (Observer and Strategy)
 * @date 2026-10-28
 * 
 */
#include "RoleBasedAssignment.h"
#include "WorkItem.h"
#include "Participant.h"

RoleBasedAssignment::RoleBasedAssignment(const std::vector<Participant*>& c, const std::string& role)
    : AssignmentStrategy(c), requiredRole(role) {}

bool RoleBasedAssignment::route(WorkItem* item) {
    if (!item) return false;
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        if (candidates[i] && candidates[i]->getRole() == requiredRole) {
            return item->assignTo(candidates[i]);
        }
    }
    return false;
}
