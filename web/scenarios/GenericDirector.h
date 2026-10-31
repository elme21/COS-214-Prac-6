// Director - determines the construction sequence for ANY scenario spec.
#ifndef WEB_GENERICDIRECTOR_H
#define WEB_GENERICDIRECTOR_H

#include "ScenarioSpec.h"

#include <map>
#include <string>

class DefinitionBuilder;
class ApprovalHandler;
class AssignmentStrategy;

/**
 * @brief The one Director of the Builder pattern used by the web demo.
 *
 * It reads a validated ScenarioSpec and calls the engine's DefinitionBuilder steps in order:
 * root stage, stages and nested sub-stages, tasks, dependencies, approval chains, assignment
 * strategies and decorators. The concrete builder is the engine's real CodeDefinitionBuilder.
 * Nothing here knows about any industry: education and healthcare are just spec files.
 */
class GenericDirector {
public:
    /// @brief Name of the root stage the director creates (spec ids may not start with '_').
    /// @return "_workflow"
    static const char* rootStageId() { return "_workflow"; }

    /// @brief Creates a director for one spec.
    /// @param spec a spec that passed parseScenarioSpec() (kept by reference)
    /// @param chains approval task id -> first handler of its chain (not owned)
    /// @param strategies task id -> assignment strategy (not owned)
    GenericDirector(const ScenarioSpec& spec, const std::map<std::string, ApprovalHandler*>& chains,
                    const std::map<std::string, AssignmentStrategy*>& strategies);

    /// @brief Calls the builder's steps for the whole spec.
    /// @param builder the concrete builder (the engine's CodeDefinitionBuilder)
    void construct(DefinitionBuilder& builder);

private:
    /// @brief Adds stages, their tasks and their sub-stages, depth first.
    /// @param builder the concrete builder
    /// @param stages the stages to add
    /// @param parent the id of the stage they belong to
    void addStages(DefinitionBuilder& builder, const std::vector<StageSpec>& stages, const std::string& parent);

    const ScenarioSpec& spec;                                    ///< The spec being built
    const std::map<std::string, ApprovalHandler*>& chains;       ///< Approval task id -> first handler
    const std::map<std::string, AssignmentStrategy*>& strategies; ///< Task id -> assignment strategy
};

#endif
