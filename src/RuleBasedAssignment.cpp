/**
 * @file RuleBasedAssignment.cpp
 * @brief Implementation of RuleBasedAssignment.
 *
 * @details For each candidate in order, every rule is evaluated; evaluation stops at the first failing rule (AND semantics).
 *
 * @author Logan (Observer and Strategy)
 * @date 2026-10-28
 * @ingroup StrategyPattern
 */
#include "RuleBasedAssignment.h"
#include "WorkItem.h"
#include "Participant.h"

RuleBasedAssignment::RuleBasedAssignment(const std::vector<Participant*>& c) : AssignmentStrategy(c) {}

void RuleBasedAssignment::addRule(Rule rule) { rules.push_back(rule); }

void RuleBasedAssignment::clearRules() { rules.clear(); }

bool RuleBasedAssignment::route(WorkItem* item) {
    if (!item) return false;
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        Participant* p = candidates[i];
        if (!p) continue;
        bool ok = true;
        for (std::size_t r = 0; r < rules.size() && ok; ++r) ok = rules[r](item, p);
        if (ok) return item->assignTo(p);
    }
    return false;
}
