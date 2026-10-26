
#ifndef WORKFLOWMANAGER_H
#define WORKFLOWMANAGER_H

#include <string>

class DefinitionRegistry;
class TaskRouter;

/**
 * @class WorkflowManager
 * @brief Facade providing a simplified interface to the TIVIDY workflow system.
 *
 * Coordinates workflow creation, execution, task completion and status retrieval.
 */
class WorkflowManager {
private:
    DefinitionRegistry* registry;
    TaskRouter* router;

public:
    /**
     * @brief Constructs the workflow manager.
     * @param registry Registry containing workflow definitions.
     * @param router Task assignment and routing service.
     */
    WorkflowManager(
        DefinitionRegistry* registry,
        TaskRouter* router);

    /**
     * @brief Creates a running workflow instance.
     * @param definitionName Name of the workflow definition.
     * @return Identifier of the created instance.
     */
    std::string createInstance(
        const std::string& definitionName);

    /**
     * @brief Starts a workflow instance.
     * @param instanceId Identifier of the instance.
     */
    void start(const std::string& instanceId);

    /**
     * @brief Completes a work item.
     * @param itemId Identifier of the work item.
     */
    void complete(const std::string& itemId);

    /**
     * @brief Gets the current status of a workflow instance.
     * @param instanceId Identifier of the instance.
     * @return Current workflow status.
     */
    std::string getStatus(
        const std::string& instanceId);
};

#endif
