// TEMPORARY - stands in for CodeDefinitionBuilder::addDependency / setApprovalChain /
// addDecorator / setAssignmentStrategy (Member 3) until those steps stop throwing
// "not integrated yet" and the definition stores dependencies and chains. Delete when that lands.
#ifndef WEB_RECORDINGDEFINITIONBUILDER_H
#define WEB_RECORDINGDEFINITIONBUILDER_H

#include "DefinitionBuilder.h"
#include "CodeDefinitionBuilder.h"
#include "NameLookup.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

/**
 * A second ConcreteBuilder for the engine's DefinitionBuilder interface.
 *
 * addStage/addTask are forwarded unchanged to the real CodeDefinitionBuilder,
 * which creates the WorkStage/ManualTask/AutomatedTask/ApprovalTask objects.
 * The steps CodeDefinitionBuilder cannot do yet are recorded here instead, so
 * EnrolmentDirector::construct runs unchanged. The recorded dependencies are
 * later handed to the engine's ResourceCoordinator (Mediator), and the chain to
 * ApprovalTask::setFirstHandler, so no workflow logic is reimplemented here.
 */
class RecordingDefinitionBuilder : public DefinitionBuilder {
public:
    struct TaskRecord {
        std::string id;
        std::string kind;   // manual | automated | approval
        std::string stage;
        TreePath path;
    };
    struct DependencyRecord {
        std::string from;   // prerequisite
        std::string to;     // dependent
    };
    struct PendingStep {
        std::string step;   // e.g. setAssignmentStrategy
        std::string task;
        std::string detail;
    };

    explicit RecordingDefinitionBuilder(const std::string& workflowName);

    void addStage(const std::string& name, const std::string& parent) override;
    void addTask(const std::string& name, const std::string& kind, const std::string& stage) override;
    void setAssignmentStrategy(const std::string& task, AssignmentStrategy* strategy) override;
    void addDecorator(const std::string& task, const std::string& decorator) override;
    void addDependency(const std::string& from, const std::string& to) override;
    void setApprovalChain(const std::string& task, ApprovalHandler* handler) override;
    std::shared_ptr<WorkflowDefinition> getDefinition() override;

    /** Display names for tasks (ID -> label). Applied with WorkItem::setName in getDefinition(). */
    void setTaskLabels(const std::map<std::string, std::string>& labels);

    const std::vector<TaskRecord>& tasks() const { return taskRecords; }
    const std::vector<DependencyRecord>& dependencies() const { return dependencyRecords; }
    const std::vector<PendingStep>& pendingSteps() const { return pending; }
    const NameLookup& names() const { return lookup; }

private:
    TreePath childPath(const std::string& parent);

    CodeDefinitionBuilder real;
    std::map<std::string, TreePath> stagePaths;
    std::map<std::string, int> childCounts;
    std::vector<TaskRecord> taskRecords;
    std::vector<DependencyRecord> dependencyRecords;
    std::vector<std::pair<std::string, ApprovalHandler*> > chains;
    std::vector<PendingStep> pending;
    std::map<std::string, std::string> labels;
    NameLookup lookup;
    bool finalised;
};

#endif
