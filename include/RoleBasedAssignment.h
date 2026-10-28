#ifndef ROLEBASEDASSIGNMENT_H
#define ROLEBASEDASSIGNMENT_H

#include <string>
#include "AssignmentStrategy.h"

/** Strategy - ConcreteStrategy. Assigns to the first candidate holding the required role. */
class RoleBasedAssignment : public AssignmentStrategy {
public:
    RoleBasedAssignment(const std::vector<Participant*>& candidates, const std::string& requiredRole);

    bool route(WorkItem* item) override;

private:
    std::string requiredRole;
};

#endif
