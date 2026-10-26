#include "../include/CodeDefinitionBuilder.h"
#include "../include/WorkStage.h"
#include "../include/ManualTask.h"
#include "../include/AutomatedTask.h"
#include "../include/ApprovalTask.h"

#include <stdexcept>
#include <memory>
#include <string>

CodeDefinitionBuilder::CodeDefinitionBuilder(const std::string& name): workflowName(name)
{
}

void CodeDefinitionBuilder::addStage(const std::string& name, const std::string& parent)
{
    if (stages.count(name) != 0 || tasks.count(name) != 0) {
        throw std::invalid_argument("Duplicate stage or task name: " + name);
    }

    if (parent.empty()) {
        if (definition) {
            throw std::logic_error("Root stage already exists");
        }
        WorkStage* root = new WorkStage();
        definition = std::make_shared<WorkflowDefinition>(workflowName, std::shared_ptr<WorkComponent>(root));
        stages[name] = root;
        return;
    }

    auto it = stages.find(parent);
    if (it == stages.end()) {
        throw std::invalid_argument("Unknown parent stage: " + parent);
    }

    WorkStage* child = new WorkStage();
    it->second->add(child); 
    stages[name] = child;
}

void CodeDefinitionBuilder::addTask(
    const std::string& name,
    const std::string& kind,
    const std::string& stage)
{
    if (stages.count(name) != 0 || tasks.count(name) != 0) {
        throw std::invalid_argument("Duplicate stage or task name: " + name);
    }

    auto it = stages.find(stage);
    if (it == stages.end()) {
        throw std::invalid_argument("Unknown stage: " + stage);
    }

    WorkItem* item = nullptr;
    if (kind == "manual") {
        item = new ManualTask();
    } else if (kind == "automated") {
        item = new AutomatedTask();
    } else if (kind == "approval") {
        item = new ApprovalTask();
    } else {
        throw std::invalid_argument("Unknown task kind: " + kind);
    }

    it->second->add(item); 
    tasks[name] = item;
}

void CodeDefinitionBuilder::setAssignmentStrategy(const std::string& task,AssignmentStrategy* strategy)
{
    // WAIT FOR: agreed task-to-strategy storage and ownership.
    (void)task; (void)strategy;
    throw std::logic_error("setAssignmentStrategy not integrated yet");
}

void CodeDefinitionBuilder::addDecorator(const std::string& task,const std::string& decorator)
{
    //WAIT FOR: decorator factory and replacement/ownership contract.
    (void)task; (void)decorator; 
    throw std::logic_error("addDecorator not integrated yet");
}

void CodeDefinitionBuilder::addDependency(const std::string& from,const std::string& to)
{
    //WAIT FOR: per-instance dependency storage (avoid shared mutable task pointers).
    (void)from; (void)to;
    throw std::logic_error("addDependency not integrated yet");
}

void CodeDefinitionBuilder::setApprovalChain(const std::string& task, ApprovalHandler* handler)
{
    //WAIT FOR: approval handler lifetime/ownership agreement.
    (void)task; (void)handler;
    throw std::logic_error("setApprovalChain not integrated yet");
}

std::shared_ptr<WorkflowDefinition>CodeDefinitionBuilder::getDefinition()
{
    if (!definition) {
        throw std::logic_error("No root stage has been added");
    }
    return definition;
}

//Please note that this still needs a lot of work.