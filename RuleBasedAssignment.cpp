#include "RuleBasedAssignment.h"
#include "WorkItem.h"
#include "Participant.h"

RuleBasedAssignment::RuleBasedAssignment(const std::vector<Participant*>& c) : AssignmentStrategy(c) {}

void RuleBasedAssignment::addRule(Rule rule) { rules.push_back(rule); }

void RuleBasedAssignment::clearRules() { rules.clear(); }

void RuleBasedAssignment::route(WorkItem* item) {
    if (!item) return;
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        Participant* p = candidates[i];
        if (!p) continue;
        bool ok = true;
        for (std::size_t r = 0; r < rules.size() && ok; ++r) ok = rules[r](item, p);
        if (ok) {
            item->assignTo(p);
            return;
        }
    }
}