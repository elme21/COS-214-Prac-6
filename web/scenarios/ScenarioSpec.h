#ifndef WEB_SCENARIOSPEC_H
#define WEB_SCENARIOSPEC_H

#include "json.hpp"

#include <map>
#include <string>
#include <vector>

/**
 * @file ScenarioSpec.h
 * @brief The generic scenario specification: plain data that describes ANY workflow.
 *
 * A spec is industry data (allowed by the spec, section 2). The generic director turns it
 * into a definition with the real CodeDefinitionBuilder; nothing in the engine or the web
 * server knows which industry a spec describes. See web/README.md for the JSON format.
 */

typedef std::map<std::string, std::string> CaseData;

/** One condition on input data: field op value. op is ==, !=, <, <=, >, >= or contains. */
struct RuleSpec {
    std::string field;
    std::string op;
    std::string value;
    std::string branch;   // decision rules only: the branch chosen when the condition holds
    bool present;
    RuleSpec() : present(false) {}
};

/** What happens after a task reaches a state (applied with real engine commands / calls). */
struct EffectSpec {
    CaseData set;                      // case data to set
    std::string outcome;               // instance outcome text; {field} is replaced by input data
    std::vector<std::string> release;  // tasks whose resource is handed back (ResourceCoordinator::release)
    std::vector<std::string> cancel;   // tasks to cancel (CancelCommand)
    std::vector<std::string> unlock;   // tasks to make available (MakeAvailableCommand)
    bool empty() const { return set.empty() && outcome.empty() && release.empty() && cancel.empty() && unlock.empty(); }
};

/** One level of an approval chain. The last level is always the final approver. */
struct ChainLevelSpec {
    std::string label;   // e.g. "Approver level 1" or "Final approver" (renamable)
    std::string role;    // who acts at this level (display)
    double limit;        // highest request value this level may approve
    ChainLevelSpec() : limit(0) {}
};

struct TaskSpec {
    std::string id;
    std::string name;
    std::string type;                  // manual | automated | approval
    std::string role;                  // responsible role
    std::string help;
    std::string stageId;
    std::vector<std::string> dependsOn;
    std::string resource;              // "", a resource id, or "@assignee" (the assignee's own resource)
    std::vector<std::string> decorators;
    std::string strategy;              // none | role | workload
    // automated
    bool succeeds;
    RuleSpec successRule;
    // approval
    double requestValue;
    std::string requestField;          // input field holding the request value (wins over requestValue)
    std::vector<ChainLevelSpec> chain;
    // effects
    EffectSpec onComplete;
    EffectSpec onReject;
    EffectSpec onCancel;
    TaskSpec() : succeeds(true), requestValue(0) {}
};

struct StageSpec {
    std::string id;
    std::string name;
    std::vector<StageSpec> stages;     // nested sub-stages (Composite)
    std::vector<TaskSpec> tasks;
};

struct InputSpec {
    std::string key;
    std::string label;
    std::string type;                  // text | number | choice
    std::vector<std::string> options;  // choice only
};

struct ParticipantSpec {
    std::string id;
    std::string name;
    std::string role;
};

struct ResourceSpec {
    std::string id;
    std::string name;
    int capacity;
    std::string owner;                 // optional participant id (used by a task's "@assignee" resource)
    ResourceSpec() : capacity(1) {}
};

struct BranchSpec {
    std::string id;
    std::string label;
    std::vector<std::string> tasks;
};

/** A routing point: after a task completes, one branch is taken and the others are cancelled. */
struct DecisionSpec {
    std::string id;
    std::string label;
    std::string after;                 // task whose completion triggers the decision
    std::string mode;                  // manual (chosen at runtime) | rule (decided by rules on input data)
    std::vector<BranchSpec> branches;
    std::vector<RuleSpec> rules;       // first matching rule picks the branch
    std::string defaultBranch;
};

struct SeedSpec {
    std::string label;
    std::string summary;
    CaseData data;
};

struct ScenarioSpec {
    std::string name;
    std::string industry;
    std::string description;
    std::string idPrefix;
    std::string subjectField;          // input field used as an instance's title
    std::vector<InputSpec> inputs;
    std::vector<ParticipantSpec> participants;
    std::vector<ResourceSpec> resources;
    std::vector<StageSpec> stages;
    std::vector<DecisionSpec> decisions;
    std::vector<SeedSpec> seeds;
    nlohmann::json source;             // the JSON it was read from

    // Look-ups (valid after parse).
    const TaskSpec* task(const std::string& id) const;
    const InputSpec* input(const std::string& key) const;
    const ParticipantSpec* participant(const std::string& id) const;
    const ResourceSpec* resource(const std::string& id) const;
    const DecisionSpec* decisionAfter(const std::string& taskId) const;
    std::vector<const TaskSpec*> allTasks() const;
};

/**
 * Reads a spec from JSON. Returns false and fills errors (one clear message each) if the JSON
 * is malformed or the spec is invalid: missing fields, duplicate or unknown ids, unknown
 * roles/resources/fields, dependency cycles, empty approval chains, decisions with fewer than
 * two branches, and so on.
 */
bool parseScenarioSpec(const nlohmann::json& json, ScenarioSpec& spec, std::vector<std::string>& errors);

#endif
