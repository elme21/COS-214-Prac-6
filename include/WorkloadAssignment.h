#ifndef WORKLOADASSIGNMENT_H
#define WORKLOADASSIGNMENT_H

#include "AssignmentStrategy.h"

/** Strategy - ConcreteStrategy. Assigns to the candidate with the lowest workload(); ties go to the earliest. */
class WorkloadAssignment : public AssignmentStrategy {
public:
    explicit WorkloadAssignment(const std::vector<Participant*>& candidates);

    bool route(WorkItem* item) override;
};

#endif
