#ifndef RULEBASEDASSIGNMENT_H
#define RULEBASEDASSIGNMENT_H

#include <functional>
#include "AssignmentStrategy.h"

/**
 * Strategy - ConcreteStrategy.
 * Assigns to the first candidate that satisfies ALL configured rules. A rule is
 * a predicate over (item, participant), so rules can be calculated from workflow
 * data and loaded per definition without writing new classes.
 */
class RuleBasedAssignment : public AssignmentStrategy {
public:
    typedef std::function<bool(WorkItem*, Participant*)> Rule;

    explicit RuleBasedAssignment(const std::vector<Participant*>& candidates);

    void route(WorkItem* item) override;

    void addRule(Rule rule);
    void clearRules();

private:
    std::vector<Rule> rules;
};

#endif