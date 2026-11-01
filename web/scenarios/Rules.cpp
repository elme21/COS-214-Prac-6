#include "Rules.h"

#include <sstream>

namespace {
bool toNumber(const std::string& s, double& value) {
    if (s.empty()) return false;
    std::istringstream in(s);
    in >> value;
    return !in.fail() && in.eof();
}

std::string valueOf(const CaseData& data, const std::string& key) {
    CaseData::const_iterator it = data.find(key);
    return it == data.end() ? std::string() : it->second;
}
}

bool ruleHolds(const RuleSpec& rule, const CaseData& data) {
    const std::string left = valueOf(data, rule.field);
    const std::string& right = rule.value;
    if (rule.op == "contains") return left.find(right) != std::string::npos;

    double a = 0, b = 0;
    if (toNumber(left, a) && toNumber(right, b)) {
        if (rule.op == "==") return a == b;
        if (rule.op == "!=") return a != b;
        if (rule.op == "<") return a < b;
        if (rule.op == "<=") return a <= b;
        if (rule.op == ">") return a > b;
        if (rule.op == ">=") return a >= b;
        return false;
    }
    if (rule.op == "==") return left == right;
    if (rule.op == "!=") return left != right;
    if (rule.op == "<") return left < right;
    if (rule.op == "<=") return left <= right;
    if (rule.op == ">") return left > right;
    if (rule.op == ">=") return left >= right;
    return false;
}

std::string describeRule(const RuleSpec& rule) {
    return rule.field + " " + rule.op + " " + rule.value;
}

std::string fillTemplate(const std::string& text, const CaseData& data) {
    std::string out;
    for (size_t i = 0; i < text.size(); ++i) {
        size_t close = text[i] == '{' ? text.find('}', i) : std::string::npos;
        if (close == std::string::npos) {
            out += text[i];
            continue;
        }
        std::string key = text.substr(i + 1, close - i - 1);
        CaseData::const_iterator it = data.find(key);
        out += it == data.end() ? "{" + key + "}" : it->second;
        i = close;
    }
    return out;
}

std::string branchByRules(const DecisionSpec& decision, const CaseData& data, std::string& reason) {
    for (size_t i = 0; i < decision.rules.size(); ++i) {
        if (ruleHolds(decision.rules[i], data)) {
            reason = "rule " + describeRule(decision.rules[i]) + " holds (" + decision.rules[i].field + " = " +
                     valueOf(data, decision.rules[i].field) + ")";
            return decision.rules[i].branch;
        }
    }
    reason = decision.rules.empty() ? std::string("no rules: default branch") : std::string("no rule holds: default branch");
    return decision.defaultBranch;
}
