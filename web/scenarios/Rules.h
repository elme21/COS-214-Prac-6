#ifndef WEB_RULES_H
#define WEB_RULES_H

#include "ScenarioSpec.h"

#include <string>

/**
 * Evaluates one condition from a spec on an instance's input data. Numbers are compared as
 * numbers when both sides parse as numbers, otherwise as text. Used for decision rules and
 * automated-step success rules; industry rules are data, not code.
 */
bool ruleHolds(const RuleSpec& rule, const CaseData& data);

/** "field op value" for display, e.g. "claimValue > 50000". */
std::string describeRule(const RuleSpec& rule);

/** Replaces each {field} in text with the input value of that field. */
std::string fillTemplate(const std::string& text, const CaseData& data);

/** The branch a decision's rules choose for this data (its default branch if none matches). */
std::string branchByRules(const DecisionSpec& decision, const CaseData& data, std::string& reason);

#endif
