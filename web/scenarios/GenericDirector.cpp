#include "GenericDirector.h"

#include "DefinitionBuilder.h"

GenericDirector::GenericDirector(const ScenarioSpec& s, const std::map<std::string, ApprovalHandler*>& c,
                                 const std::map<std::string, AssignmentStrategy*>& st)
    : spec(s), chains(c), strategies(st) {}

void GenericDirector::addStages(DefinitionBuilder& builder, const std::vector<StageSpec>& stages,
                                const std::string& parent) {
    for (size_t i = 0; i < stages.size(); ++i) {
        builder.addStage(stages[i].id, parent);
        for (size_t t = 0; t < stages[i].tasks.size(); ++t) {
            builder.addTask(stages[i].tasks[t].id, stages[i].tasks[t].type, stages[i].id);
        }
        addStages(builder, stages[i].stages, stages[i].id);   // nested sub-stages (Composite)
    }
}

void GenericDirector::construct(DefinitionBuilder& builder) {
    builder.addStage(rootStageId(), "");
    addStages(builder, spec.stages, rootStageId());

    std::vector<const TaskSpec*> tasks = spec.allTasks();
    for (size_t i = 0; i < tasks.size(); ++i) {
        for (size_t d = 0; d < tasks[i]->dependsOn.size(); ++d) {
            builder.addDependency(tasks[i]->dependsOn[d], tasks[i]->id);
        }
    }
    for (std::map<std::string, ApprovalHandler*>::const_iterator it = chains.begin(); it != chains.end(); ++it) {
        builder.setApprovalChain(it->first, it->second);
    }
    for (std::map<std::string, AssignmentStrategy*>::const_iterator it = strategies.begin(); it != strategies.end(); ++it) {
        builder.setAssignmentStrategy(it->first, it->second);
    }
    // Decorators last: addDecorator replaces the task inside its stage with the decorator.
    for (size_t i = 0; i < tasks.size(); ++i) {
        for (size_t d = 0; d < tasks[i]->decorators.size(); ++d) {
            builder.addDecorator(tasks[i]->id, tasks[i]->decorators[d]);
        }
    }
}
