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

/// @brief One level of a built approval chain, for display.
struct ChainLevelInfo {
    std::string label;   ///< Handler name
    std::string role;    ///< Role at this level
    double limit;        ///< Highest request value this level approves
    bool isFinal;        ///< True for the FinalApproverHandler (rejects anything above its limit)
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
    /// @brief Creates the participants, resources, approval chains and strategies a spec asks for.
    /// @param spec a spec that passed parseScenarioSpec()
    explicit ScenarioRuntime(const ScenarioSpec& spec);
    /// @brief Deletes the engine objects this scenario owns.
    ~ScenarioRuntime();

    /// @brief Builds the definition with the GenericDirector and registers it.
    /// @param registry the real DefinitionRegistry
    /// @param error receives a message if registration fails
    /// @param buildSteps receives one line per builder step, for display
    /// @return true if the definition was built and registered
    bool build(DefinitionRegistry& registry, std::string& error, std::vector<std::string>& buildSteps);

    /// @brief Gets the spec.
    /// @return the spec this scenario was made from
    const ScenarioSpec& spec() const { return scenario; }
    /// @brief Gets the scenario key (made from the name, e.g. "patient-booking").
    /// @return the key
    const std::string& key() const { return slug; }
    /// @brief Gets the registered definition.
    /// @return the definition, or nullptr before build()
    const WorkflowDefinition* definition() const { return built; }
    /// @brief Gets the stage names by tree position (stand-in, see StageNames).
    /// @return the stage names
    const StageNames& stageNames() const { return names; }
    /// @brief Counts a new instance of this scenario.
    /// @return the next instance number (1, 2, ...)
    int nextInstanceNumber() { return ++counter; }

    /// @brief Finds a participant.
    /// @param id participant id
    /// @return the real Participant, or nullptr if unknown
    Participant* participant(const std::string& id) const;
    /// @brief Gets every participant.
    /// @return the real Participants, in spec order
    const std::vector<std::unique_ptr<Participant> >& participants() const { return people; }
    /// @brief Finds a resource.
    /// @param id resource id
    /// @return the real Resource, or nullptr if unknown
    Resource* resource(const std::string& id) const;
    /// @brief The resource a task needs when assigned to this participant.
    /// @param taskId the task id
    /// @param participantId the assignee
    /// @return the resource, or nullptr if the task needs none or the assignee owns none ("@assignee")
    Resource* resourceFor(const std::string& taskId, const std::string& participantId) const;
    /// @brief Finds the spec id of a resource.
    /// @param resource the real Resource
    /// @return its id, or "" if it is not one of this scenario's resources
    std::string resourceIdOf(const Resource* resource) const;

    /// @brief Gets the assignment strategy a task uses for auto-assign.
    /// @param taskId the task id
    /// @return the real strategy, or nullptr if the task has none
    AssignmentStrategy* strategyFor(const std::string& taskId) const;
    /// @brief Gets the class name of a task's strategy, for display.
    /// @param taskId the task id
    /// @return "RoleBasedAssignment", "WorkloadAssignment", or "" if the task has no strategy
    std::string strategyName(const std::string& taskId) const;
    /// @brief Describes a task's approval chain.
    /// @param taskId the approval task id
    /// @return one entry per level, first level first (empty if the task has no chain)
    std::vector<ChainLevelInfo> chainOf(const std::string& taskId) const;

private:
    ScenarioSpec scenario;                                             ///< The spec
    std::string slug;                                                  ///< The scenario key
    std::vector<std::unique_ptr<Participant> > people;                 ///< Real participants
    std::vector<std::unique_ptr<Resource> > resources;                 ///< Real resources
    std::vector<std::unique_ptr<ApprovalHandler> > handlers;          ///< The real approval handlers
    std::map<std::string, ApprovalHandler*> chainHeads;                ///< Approval task id -> first handler of its chain
    std::map<std::string, std::vector<ChainLevelInfo> > chainInfo;     ///< Approval task id -> its levels, for display
    std::vector<std::unique_ptr<AssignmentStrategy> > strategyObjects; ///< Real strategies
    std::map<std::string, AssignmentStrategy*> strategies;             ///< Task id -> its strategy
    std::unique_ptr<CodeDefinitionBuilder> builder;                    ///< The real builder used by build()
    const WorkflowDefinition* built;                                   ///< The registered definition, or nullptr
    StageNames names;                                                  ///< Stage names by tree position
    int counter;                                                       ///< Instances created so far
};

#endif
