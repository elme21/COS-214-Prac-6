#ifndef ASSIGNMENTSTRATEGY_H
#define ASSIGNMENTSTRATEGY_H

#include <string>
#include <vector>

class WorkItem;      // Person 2
class Participant;   // Person 4

/**
 * @file AssignmentStrategy.h
 * @brief Strategy interface for deciding who gets a work item.
 *
 * @details Separates "which rule picks the participant" from "what a work item is", so an
 * organisation can change its assignment policy without touching WorkItem or TaskRouter.
 *
 * @author Logan (Observer and Strategy)
 * @date 2026-10-28
 * 
 */

/**
 * @brief Strategy: one rule for choosing who receives a work item.
 *
 * route(item) chooses a participant from the candidates it was given and calls
 * WorkItem::assignTo() on the item. The item's state machine may refuse
 * (e.g. the item is not available yet); route() reports that through its result.
 *
 * @par Adding a new rule
 * Derive from AssignmentStrategy, override route(), and pass the new object to TaskRouter::setStrategy().
 * No existing class changes.
 *
 * @invariant Strategies never own the participants or the item.
 * 
 * @see TaskRouter, RoleBasedAssignment, WorkloadAssignment, RuleBasedAssignment
 */
class AssignmentStrategy {
public:
    explicit AssignmentStrategy(const std::vector<Participant*>& candidates)
        : candidates(candidates) {}
    virtual ~AssignmentStrategy() {}

    virtual bool route(WorkItem* item) = 0;

    /** Replace the pool of participants this rule chooses from. */
    void setCandidates(const std::vector<Participant*>& c) { candidates = c; }

protected:
    std::vector<Participant*> candidates;
};

#endif
