#ifndef RULEBASEDASSIGNMENT_H
#define RULEBASEDASSIGNMENT_H

#include <functional>
#include "AssignmentStrategy.h"

/**
 * @class RuleBasedAssignment
 * @brief Assigns a work item to the first candidate satisfying all configured rules.
 *
 * Design pattern: Strategy (ConcreteStrategy).
 *
 * A rule is a predicate over a (work item, participant) pair, so rules can be
 * calculated from workflow data and loaded per workflow definition without
 * writing new classes. A candidate is selected only if every rule returns
 * @c true for it; candidates are evaluated in order.
 *
 * @par Example
 * @code
 * RuleBasedAssignment strategy(candidates);
 * strategy.addRule([](WorkItem* item, Participant* p) {
 *     return p->isAvailable();
 * });
 * strategy.route(item);
 * */
class RuleBasedAssignment : public AssignmentStrategy {
public:
/**
     * @brief Predicate deciding whether a participant may receive a work item.
     *
     * Receives the work item being routed and the candidate under evaluation,
     * and returns @c true if the candidate is acceptable.
     */
    typedef std::function<bool(WorkItem*, Participant*)> Rule;

 /**
     * @brief Constructs a rule-based assignment strategy with no rules.
     *
     * @param candidates Participants eligible to receive work items, in
     *                   priority order. The strategy does not take ownership
     *                   of the pointers
     * */
    explicit RuleBasedAssignment(const std::vector<Participant*>& candidates);

/**
     * @brief Routes a work item to the first candidate satisfying all rules.
     *
     * @param item The work item to assign. Must not be null.
     * */
    bool route(WorkItem* item) override;

/**
     * @brief Adds a rule that candidates must satisfy.
     *
     * Rules are combined with logical AND: a candidate must pass every rule
     * to be selected.
     *
     * @param rule The predicate to add.
     */

    void addRule(Rule rule);
/**
     * @brief Removes all configured rules.
     */
    void clearRules();

private:
    std::vector<Rule> rules;
};

#endif
