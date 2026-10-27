// TEMPORARY - stands in for CodeDefinitionBuilder::addDependency / setApprovalChain /
// addDecorator / setAssignmentStrategy (Member 3) until those steps stop throwing
// "not integrated yet" and the definition stores dependencies and chains. Delete when that lands.
#include "RecordingDefinitionBuilder.h"

#include "WorkflowDefinition.h"
#include "WorkComponent.h"
#include "WorkItem.h"
#include "ApprovalTask.h"

#include <stdexcept>

RecordingDefinitionBuilder::RecordingDefinitionBuilder(const std::string& workflowName)
    : real(workflowName), finalised(false) {}

TreePath RecordingDefinitionBuilder::childPath(const std::string& parent) {
    TreePath path = stagePaths[parent];
    path.push_back(childCounts[parent]++);
    return path;
}

void RecordingDefinitionBuilder::addStage(const std::string& name, const std::string& parent) {
    real.addStage(name, parent);   // validates and creates the WorkStage
    TreePath path = parent.empty() ? TreePath() : childPath(parent);
    stagePaths[name] = path;
    lookup.setStageName(path, name);
}

void RecordingDefinitionBuilder::addTask(const std::string& name, const std::string& kind,
                                         const std::string& stage) {
    real.addTask(name, kind, stage);   // validates and creates the task
    TaskRecord record;
    record.id = name;
    record.kind = kind;
    record.stage = stage;
    record.path = childPath(stage);
    taskRecords.push_back(record);
}

void RecordingDefinitionBuilder::setAssignmentStrategy(const std::string& task, AssignmentStrategy*) {
    PendingStep step;
    step.step = "setAssignmentStrategy";
    step.task = task;
    step.detail = "recorded only: TaskRouter / AssignmentStrategy (Member 6) not integrated";
    pending.push_back(step);
}

void RecordingDefinitionBuilder::addDecorator(const std::string& task, const std::string& decorator) {
    PendingStep step;
    step.step = "addDecorator";
    step.task = task;
    step.detail = "recorded only: '" + decorator + "' has a header but no implementation yet";
    pending.push_back(step);
}

void RecordingDefinitionBuilder::addDependency(const std::string& from, const std::string& to) {
    DependencyRecord record;
    record.from = from;
    record.to = to;
    dependencyRecords.push_back(record);
}

void RecordingDefinitionBuilder::setApprovalChain(const std::string& task, ApprovalHandler* handler) {
    chains.push_back(std::make_pair(task, handler));
}

void RecordingDefinitionBuilder::setTaskLabels(const std::map<std::string, std::string>& taskLabels) {
    labels = taskLabels;
}

std::shared_ptr<WorkflowDefinition> RecordingDefinitionBuilder::getDefinition() {
    std::shared_ptr<WorkflowDefinition> definition = real.getDefinition();
    if (finalised) return definition;
    finalised = true;

    WorkComponent* root = definition->getRoot().get();

    // Name and ID every task created by CodeDefinitionBuilder (Member 2 setters).
    // Clones copy them, so every instance inherits the same IDs.
    for (size_t i = 0; i < taskRecords.size(); ++i) {
        WorkItem* item = asWorkItem(componentAt(root, taskRecords[i].path));
        if (item == nullptr) {
            throw std::logic_error("RecordingDefinitionBuilder: no task at " + pathKey(taskRecords[i].path));
        }
        item->setId(taskRecords[i].id);
        std::map<std::string, std::string>::const_iterator label = labels.find(taskRecords[i].id);
        item->setName(label == labels.end() ? taskRecords[i].id : label->second);
    }

    // Attach approval chains with the real ApprovalTask setter. Clones copy the
    // (non-owning) handler pointer, so every instance uses the same chain.
    for (size_t i = 0; i < chains.size(); ++i) {
        ApprovalTask* task = asApprovalTask(findTask(root, chains[i].first));
        if (task == nullptr) {
            throw std::logic_error("RecordingDefinitionBuilder: no approval task " + chains[i].first);
        }
        task->setFirstHandler(chains[i].second);
    }
    return definition;
}
