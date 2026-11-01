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

/// @brief Case data of one instance (input field or decision id -> value), as stored in WorkflowInstance.
typedef std::map<std::string, std::string> CaseData;

/// @brief One condition on input data: field op value. op is ==, !=, <, <=, >, >= or contains.
struct RuleSpec {
    std::string field;    ///< Input field the rule reads
    std::string op;       ///< Comparison: ==, !=, <, <=, >, >= or contains
    std::string value;    ///< Value the field is compared with
    std::string branch;   ///< Decision rules only: the branch chosen when the condition holds
    bool present;         ///< True if the spec contained this rule (a field was given)
    /// @brief Creates an absent rule.
    RuleSpec() : present(false) {}
};

/// @brief What happens after a task reaches a state (applied with real engine commands / calls).
struct EffectSpec {
    CaseData set;                      ///< Case data to set
    std::string outcome;               ///< Instance outcome text; {field} is replaced by input data
    std::vector<std::string> release;  ///< Tasks whose resource is handed back (ResourceCoordinator::release)
    std::vector<std::string> cancel;   ///< Tasks to cancel (CancelCommand)
    std::vector<std::string> unlock;   ///< Tasks to make available (MakeAvailableCommand)
    /// @brief Checks whether this effect does nothing.
    /// @return true if no part of the effect is set
    bool empty() const { return set.empty() && outcome.empty() && release.empty() && cancel.empty() && unlock.empty(); }
};

/// @brief One level of an approval chain. The last level is always the final approver.
struct ChainLevelSpec {
    std::string label;   ///< Handler name, e.g. "Approver level 1" or "Final approver" (renamable)
    std::string role;    ///< Who acts at this level (passed to RoleApproverHandler; display for the final level)
    double limit;        ///< Highest request value this level may approve
    /// @brief Creates a level with limit 0.
    ChainLevelSpec() : limit(0) {}
};

/// @brief One task (a WorkItem in the engine) and everything the spec says about it.
struct TaskSpec {
    std::string id;                        ///< Unique task id (also the WorkItem id)
    std::string name;                      ///< Display name
    std::string type;                      ///< manual | automated | approval
    std::string role;                      ///< Responsible role
    std::string help;                      ///< Instructions shown to the person doing the task
    std::string stageId;                   ///< Id of the stage that contains the task
    std::vector<std::string> dependsOn;    ///< Tasks that must complete first (Mediator dependencies)
    std::string resource;                  ///< "", a resource id, or "@assignee" (the assignee's own resource)
    std::vector<std::string> decorators;   ///< Decorators to add with CodeDefinitionBuilder::addDecorator ("audit")
    std::string strategy;                  ///< Assignment strategy: none | role | workload
    bool succeeds;                         ///< Automated tasks: AutomatedTask::setStepSucceeds value when no rule is given
    RuleSpec successRule;                  ///< Automated tasks: the step succeeds only if this rule holds
    double requestValue;                   ///< Approval tasks: fixed value sent down the chain
    std::string requestField;              ///< Approval tasks: input field holding the request value (wins over requestValue)
    std::vector<ChainLevelSpec> chain;     ///< Approval tasks: the approval chain, first level first
    EffectSpec onComplete;                 ///< Applied when the task completes
    EffectSpec onReject;                   ///< Applied when the task is rejected
    EffectSpec onCancel;                   ///< Applied when the task is cancelled
    /// @brief Creates a task that succeeds by default and has request value 0.
    TaskSpec() : succeeds(true), requestValue(0) {}
};

/// @brief One stage (a WorkStage in the engine): tasks plus nested sub-stages (Composite).
struct StageSpec {
    std::string id;                    ///< Unique stage id
    std::string name;                  ///< Display name
    std::vector<StageSpec> stages;     ///< Nested sub-stages (Composite)
    std::vector<TaskSpec> tasks;       ///< Tasks directly in this stage
};

/// @brief One piece of information collected for each case.
struct InputSpec {
    std::string key;                   ///< Field key in the case data
    std::string label;                 ///< Display label
    std::string type;                  ///< text | number | choice
    std::vector<std::string> options;  ///< Choice only: the allowed values
};

/// @brief One person taking part (a Participant in the engine).
struct ParticipantSpec {
    std::string id;     ///< Unique participant id
    std::string name;   ///< Display name
    std::string role;   ///< Role, matched against task and chain roles
};

/// @brief One limited resource (a Resource colleague of the Mediator).
struct ResourceSpec {
    std::string id;                    ///< Unique resource id
    std::string name;                  ///< Display name
    int capacity;                      ///< How many tasks may hold it at once
    std::string owner;                 ///< Optional participant id (used by a task's "@assignee" resource)
    /// @brief Creates a resource with capacity 1.
    ResourceSpec() : capacity(1) {}
};

/// @brief One path of a decision: the tasks that run if it is taken.
struct BranchSpec {
    std::string id;                    ///< Branch id, unique within its decision
    std::string label;                 ///< Display label
    std::vector<std::string> tasks;    ///< Tasks on this path; the tasks of the other paths are cancelled
};

/// @brief A routing point: after a task completes, one branch is taken and the others are cancelled.
struct DecisionSpec {
    std::string id;                    ///< Decision id; the chosen branch is stored in the case data under it
    std::string label;                 ///< Display label
    std::string after;                 ///< Task whose completion triggers the decision
    std::string mode;                  ///< manual (chosen at runtime) | rule (decided by rules on input data)
    std::vector<BranchSpec> branches;  ///< The paths (at least two)
    std::vector<RuleSpec> rules;       ///< First matching rule picks the branch (a suggestion in manual mode)
    std::string defaultBranch;         ///< Branch taken when no rule matches (defaults to the first branch)
};

/// @brief A ready-made example case.
struct SeedSpec {
    std::string label;     ///< Display name of the example
    std::string summary;   ///< One-line description
    CaseData data;         ///< Input values
};

/// @brief A complete scenario: inputs, people, resources, stages, decisions and example cases.
struct ScenarioSpec {
    std::string name;                           ///< Workflow name (also the definition name in the registry)
    std::string industry;                       ///< Industry label
    std::string description;                   ///< Short description
    std::string idPrefix;                       ///< Case id prefix (e.g. "EDU" gives EDU-1); made from the name if empty
    std::string subjectField;                   ///< Input field used as an instance's title
    std::vector<InputSpec> inputs;              ///< Information collected for each case
    std::vector<ParticipantSpec> participants;  ///< People and their roles
    std::vector<ResourceSpec> resources;        ///< Limited resources
    std::vector<StageSpec> stages;              ///< Top-level stages
    std::vector<DecisionSpec> decisions;        ///< Routing points
    std::vector<SeedSpec> seeds;                ///< Example cases
    nlohmann::json source;                      ///< The JSON it was read from

    /// @brief Finds a task anywhere in the stage tree.
    /// @param id task id
    /// @return the task, or nullptr if there is none
    const TaskSpec* task(const std::string& id) const;
    /// @brief Finds an input field.
    /// @param key field key
    /// @return the field, or nullptr if there is none
    const InputSpec* input(const std::string& key) const;
    /// @brief Finds a participant.
    /// @param id participant id
    /// @return the participant, or nullptr if there is none
    const ParticipantSpec* participant(const std::string& id) const;
    /// @brief Finds a resource.
    /// @param id resource id
    /// @return the resource, or nullptr if there is none
    const ResourceSpec* resource(const std::string& id) const;
    /// @brief Finds the decision made when a task completes.
    /// @param taskId the task the decision follows
    /// @return the decision, or nullptr if no decision follows that task
    const DecisionSpec* decisionAfter(const std::string& taskId) const;
    /// @brief Lists every task in the stage tree.
    /// @return pointers into this spec, sub-stages first within each stage
    std::vector<const TaskSpec*> allTasks() const;
};

/**
 * @brief Reads and validates a spec from JSON.
 *
 * Fails with one clear message per problem if the JSON is malformed or the spec is invalid:
 * missing fields, duplicate or unknown ids, unknown roles/resources/fields, dependency cycles,
 * empty approval chains, decisions with fewer than two branches, and so on.
 * @param json the spec as JSON
 * @param spec receives the parsed spec
 * @param errors receives one message per problem
 * @return true if the spec is valid
 */
bool parseScenarioSpec(const nlohmann::json& json, ScenarioSpec& spec, std::vector<std::string>& errors);

#endif
