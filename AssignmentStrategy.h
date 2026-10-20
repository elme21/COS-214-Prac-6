#ifndef ASSIGNMENTSTRATEGY_H
#define ASSIGNMENTSTRATEGY_H

#include <string>
#include <vector>

class WorkItem;      // Person 2
class Participant;   // Person 4

/**
 * Strategy pattern - Strategy.
 * Matches the class diagram: route(item) : void. A strategy chooses a
 * participant from the candidates it was given and assigns the item to them
 * (item->assignTo). If nobody is suitable the item is left unassigned.
 */
class AssignmentStrategy {
public:
    explicit AssignmentStrategy(const std::vector<Participant*>& candidates)
        : candidates(candidates) {}
    virtual ~AssignmentStrategy() {}

    virtual void route(WorkItem* item) = 0;

    /** Replace the pool of participants this rule chooses from. */
    void setCandidates(const std::vector<Participant*>& c) { candidates = c; }

protected:
    std::vector<Participant*> candidates;
};

#endif