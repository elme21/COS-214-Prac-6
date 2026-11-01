#ifndef WEB_RULES_H
#define WEB_RULES_H

#include "ScenarioSpec.h"

#include <string>

/**
 * @file Rules.h
 * @brief Evaluates the rules written in a scenario spec. Industry rules are data, not code.
 */

/**
 * @brief Evaluates one condition from a spec on an instance's case data.
 *
 * Numbers are compared as numbers when both sides parse as numbers, otherwise as text. Used for
 * decision rules and automated-step success rules.
 * @param rule the condition
 * @param data the case data
 * @return true if the condition holds
 */
bool ruleHolds(const RuleSpec& rule, const CaseData& data);

/// @brief Writes a rule for display.
/// @param rule the condition
/// @return "field op value", e.g. "claimValue > 50000"
std::string describeRule(const RuleSpec& rule);

/// @brief Replaces each {field} in a text with that field's value from the case data.
/// @param text the text, e.g. "Paid R{claimValue}"
/// @param data the case data
/// @return the filled-in text
std::string fillTemplate(const std::string& text, const CaseData& data);

/// @brief Chooses a decision's branch from its rules.
/// @param decision the decision
/// @param data the case data
/// @param reason receives which rule matched, or that the default was used
/// @return the branch id of the first matching rule, or the default branch if none matches
std::string branchByRules(const DecisionSpec& decision, const CaseData& data, std::string& reason);

#endif
