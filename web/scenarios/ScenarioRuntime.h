#ifndef WEB_SCENARIORUNTIME_H
#define WEB_SCENARIORUNTIME_H

#include "ScenarioSpec.h"
#include "StageNames.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

class Participant;
class Resource;
class ApprovalHandler;
class AssignmentStrategy;
class DefinitionRegistry;
class WorkflowDefinition;
class CodeDefinitionBuilder;

/** One level of a built approval chain, for display. */
struct ChainLevelInfo {
    std::string label;
    std::string role;
    double limit;
    bool isFinal;
};

/**
 * @brief Everything one scenario needs at run time, created from its spec.
 *
 * Owns the real engine objects the spec asks for: Participant and Resource colleagues, the
 * approval chains (RoleApproverHandler ... FinalApproverHandler), and the assignment strategies
 * (RoleBasedAssignment / WorkloadAssignment). build() runs the GenericDirector on the real
 * CodeDefinitionBuilder and registers the result in the real DefinitionRegistry.
 */
class ScenarioRuntime {
public:
    explicit ScenarioRuntime(const ScenarioSpec& spec);
    ~ScenarioRuntime();

    /** Builds the definition and registers it. Returns false with a message if registration fails. */
    bool build(DefinitionRegistry& registry, std::string& error, std::vector<std::string>& buildSteps);

    const ScenarioSpec& spec() const { return scenario; }
    const std::string& key() const { return slug; }
    const WorkflowDefinition* definition() const { return built; }
    const StageNames& stageNames() const { return names; }
    int nextInstanceNumber() { return ++counter; }

    Participant* participant(const std::string& id) const;
    const std::vector<std::unique_ptr<Participant> >& participants() const { return people; }
    Resource* resource(const std::string& id) const;
    /** The resource a task needs when assigned to this participant, or null. */
    Resource* resourceFor(const std::string& taskId, const std::string& participantId) const;
    std::string resourceIdOf(const Resource* resource) const;

    AssignmentStrategy* strategyFor(const std::string& taskId) const;
    std::string strategyName(const std::string& taskId) const;
    std::vector<ChainLevelInfo> chainOf(const std::string& taskId) const;

private:
    ScenarioSpec scenario;
    std::string slug;
    std::vector<std::unique_ptr<Participant> > people;
    std::vector<std::unique_ptr<Resource> > resources;
    std::vector<std::unique_ptr<ApprovalHandler> > handlers;          // the real handlers
    std::vector<std::string> finalNames;                              // FinalApproverHandler takes a std::string&
    std::map<std::string, ApprovalHandler*> chainHeads;
    std::map<std::string, std::vector<ChainLevelInfo> > chainInfo;
    std::vector<std::unique_ptr<AssignmentStrategy> > strategyObjects;
    std::map<std::string, AssignmentStrategy*> strategies;
    std::unique_ptr<CodeDefinitionBuilder> builder;
    const WorkflowDefinition* built;
    StageNames names;
    int counter;
};

#endif
