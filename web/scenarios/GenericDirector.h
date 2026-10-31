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
    /** Name of the root stage the director creates (spec ids may not start with '_'). */
    static const char* rootStageId() { return "_workflow"; }

    /**
     * @param spec        a spec that passed parseScenarioSpec()
     * @param chains      approval task id -> first handler of its chain (not owned)
     * @param strategies  task id -> assignment strategy (not owned)
     */
    GenericDirector(const ScenarioSpec& spec, const std::map<std::string, ApprovalHandler*>& chains,
                    const std::map<std::string, AssignmentStrategy*>& strategies);

    void construct(DefinitionBuilder& builder);

private:
    void addStages(DefinitionBuilder& builder, const std::vector<StageSpec>& stages, const std::string& parent);

    const ScenarioSpec& spec;
    const std::map<std::string, ApprovalHandler*>& chains;
    const std::map<std::string, AssignmentStrategy*>& strategies;
};

#endif
