#ifndef ROLEBASEDASSIGNMENT_H
#define ROLEBASEDASSIGNMENT_H

#include <string>
#include "AssignmentStrategy.h"

/**
 * @class RoleBasedAssignment
 * @brief Assigns a work item to the first candidate holding the required role.
 *
 * Design pattern: Strategy (ConcreteStrategy).
 *
 * The strategy scans its candidate list in order and routes the work item to
 * the first participant whose role matches @ref requiredRole.
 ***/
class RoleBasedAssignment : public AssignmentStrategy {
public:
/**
     * @brief Constructs a role-based assignment strategy.
     *
     * @param candidates   Participants eligible to receive work items, in
     *                     priority order. The strategy does not take ownership
     *                     of the pointers.
     * @param requiredRole Name of the role a participant must hold in order
     *                     to be selected.
     */
    RoleBasedAssignment(const std::vector<Participant*>& candidates, const std::string& requiredRole);

  /**
     * @brief Routes a work item to the first candidate holding the required role.
     *
     * @param item The work item to assign. Must not be null.
     *
     * @note If no candidate holds the required role, the item is left
     *       unassigned.
     */
    bool route(WorkItem* item) override;

private:
    std::string requiredRole;
};

#endif
