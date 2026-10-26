#include "include/WorkflowManager.h"

#include <stdexcept>
#include <string>

/**
 * @brief Constructs the WorkflowManager facade.
 */
WorkflowManager::WorkflowManager(DefinitionRegistry* registry,
    TaskRouter* router): registry(registry), router(router)
{
    if (registry == nullptr) {
        throw std::invalid_argument("DefinitionRegistry cannot be null");
    }

    if (router == nullptr) {
        throw std::invalid_argument("TaskRouter cannot be null");
    }
}

/**
 * @brief Creates a new workflow instance from a definition.
 */
std::string WorkflowManager::createInstance(
    const std::string& definitionName)
{
    // WAIT FOR: Prototype / DefinitionRegistry
    // Need to know how to retrieve a workflow definition.

    // WAIT FOR: WorkflowInstance
    // Need to know how to construct an independent instance using the definition's cloned root.

    (void)definitionName;
    throw std::logic_error("createInstance: waiting for integration");

}

/**
 * @brief Starts an existing workflow instance.
 */
void WorkflowManager::start(
    const std::string& instanceId)
{
    // WAIT FOR: WorkflowInstance / State
    // Need to know how an instance is started and how its lifecycle state is updated.

    (void)instanceId;
    throw std::logic_error("start: waiting for integration");

}

/**
 * @brief Completes a work item and progresses its workflow.
 */
void WorkflowManager::complete(
    const std::string& itemId)
{
    // WAIT FOR: WorkItem / WorkflowInstance
    // Need to know how to find a work item by ID and how task completion is performed.

    // WAIT FOR: ResourceCoordinator / TaskRouter
    // Need to know how completing a task unlocks
    // dependent tasks and triggers routing.
    (void)itemId;
    throw std::logic_error("complete: waiting for integration");

}

/**
 * @brief Retrieves the status of a workflow instance.
 */
std::string WorkflowManager::getStatus(
    const std::string& instanceId)
{
    // WAIT FOR: WorkflowInstance / State
    // Need to know how status is represented and retrieved from a workflow instance.


    (void)instanceId;
    throw std::logic_error("getStatus: waiting for integration");

}