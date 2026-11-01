// TEMPORARY - stands in for task lookup by ID on WorkflowManager (Member 7). The Facade keeps each
// instance's Runtime::components / Runtime::items maps private. Delete this when the Facade exposes
// an instance's tasks. It walks the tree the same way WorkflowManager::createInstance does.
#ifndef WEB_TASKLOOKUP_H
#define WEB_TASKLOOKUP_H

#include <string>
#include <vector>

class WorkComponent;
class WorkItem;
class AutomatedTask;
class ApprovalTask;

/// @brief One task in a tree: the component commands act on, and the WorkItem inside it.
struct TaskRef {
    WorkComponent* node;                   ///< The OUTERMOST component (a decorator if decorated); commands act on it so decorator hooks run
    WorkItem* item;                        ///< The WorkItem inside all decorators: identity, isComplete(), Mediator and events use it
    std::vector<std::string> decorators;   ///< Decorator class names, outermost first, e.g. {"AuditDecorator"}
};

/// @brief Unwraps one tree node with WorkComponentDecorator::getWrappedComponent().
/// @param node the node
/// @return the task reference; item is null for a stage
TaskRef refFor(WorkComponent* node);

/// @brief Lists every task, found with the tree's own iterator (createIterator()).
/// @param root the root of an instance or definition
/// @return the tasks in depth-first order
std::vector<TaskRef> allTasks(WorkComponent* root);

/// @brief Finds a task by its definition-level ID.
/// @param root the root of an instance or definition
/// @param taskId the task id
/// @return the task; node and item are null if there is none
TaskRef findTask(WorkComponent* root, const std::string& taskId);

/// @brief Gets the AutomatedTask behind a task reference.
/// @param ref the task
/// @return the AutomatedTask, or nullptr if the task is of another kind
AutomatedTask* asAutomatedTask(const TaskRef& ref);
/// @brief Gets the ApprovalTask behind a task reference.
/// @param ref the task
/// @return the ApprovalTask, or nullptr if the task is of another kind
ApprovalTask* asApprovalTask(const TaskRef& ref);

#endif
