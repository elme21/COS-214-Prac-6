#ifndef WORKFLOWMANAGER_H
#define WORKFLOWMANAGER_H

#include <string>
#include <map>
#include <memory>

class DefinitionRegistry;
class TaskRouter;
class WorkflowInstance;
class WorkflowDefinition;
class WorkComponent;
class WorkItem;
class ResourceCoordinator;
class CommandProcessor;

/// @class WorkflowManager
/// @brief Facade providing a simplified interface to the TIVIDY workflow system.
/// Coordinates workflow creation, execution, task completion and status retrieval.
class WorkflowManager {
private:
    DefinitionRegistry* registry;
    TaskRouter* router;

    // Keep the running data for each independent workflow instance.
    struct Runtime {
        std::shared_ptr<WorkflowInstance> instance;
        const WorkflowDefinition* definition;
        std::shared_ptr<ResourceCoordinator> mediator;
        std::shared_ptr<CommandProcessor> commands;

        std::map<std::string, WorkComponent*> components;
        std::map<std::string, WorkItem*> items;
    };

    std::map<std::string, Runtime> instances;
    unsigned long nextId = 1;

    void completeInInstance(const std::string& instanceId, const std::string& taskId);

public:
    /// @brief Constructs the workflow manager.
    /// @param registry Registry containing workflow definitions.
    /// @param router Task assignment and routing service.
    WorkflowManager(
        DefinitionRegistry* registry,
        TaskRouter* router);

    /// @brief Creates a new workflow instance in the Created state.
    /// @param definitionName Name of the workflow definition.
    /// @return Identifier of the created instance.
    std::string createInstance(const std::string& definitionName);

    /// @brief Starts a workflow instance.
    /// @param instanceId Identifier of the instance.
    void start(const std::string& instanceId);

    /// @brief Completes a task using a combined instance and task identifier.
    /// @param itemId Identifier in the format instanceId:taskId.
    void complete(const std::string& itemId);

    /// @brief Completes a task in the specified workflow instance.
    /// @param instanceId Identifier of the instance.
    /// @param taskId Identifier of the task within that instance.
    void complete(const std::string& instanceId, const std::string& taskId);

    /// @brief Gets the current status of a workflow instance.
    /// @param instanceId Identifier of the instance.
    /// @return Current workflow status.
    std::string getStatus( const std::string& instanceId);
};

#endif