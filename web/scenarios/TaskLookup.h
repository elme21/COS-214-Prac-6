// TEMPORARY - stands in for task lookup by ID on WorkflowManager (Member 7) until the Facade
// exposes an instance's tasks (its Runtime::components / Runtime::items maps are private).
// Delete when that lands. It walks the tree the same way WorkflowManager::createInstance does.
#ifndef WEB_TASKLOOKUP_H
#define WEB_TASKLOOKUP_H

#include <string>
#include <vector>

class WorkComponent;
class WorkItem;
class AutomatedTask;
class ApprovalTask;

/**
 * One task in a tree.
 *  - node: the OUTERMOST component (a decorator if the task is decorated). Commands act on
 *          this, so decorator hooks run.
 *  - item: the WorkItem inside all decorators. Identity (getId/getName) and isComplete()
 *          come from here, and the Mediator and events need it.
 */
struct TaskRef {
    WorkComponent* node;
    WorkItem* item;
    std::vector<std::string> decorators;   // outermost first, e.g. {"AuditDecorator"}
};

/** Unwraps one tree node (item is null for a stage). */
TaskRef refFor(WorkComponent* node);

/** Every task in depth-first order, found with the tree's own iterator (createIterator()). */
std::vector<TaskRef> allTasks(WorkComponent* root);

/** The task with this definition-level ID; node and item are null if there is none. */
TaskRef findTask(WorkComponent* root, const std::string& taskId);

AutomatedTask* asAutomatedTask(const TaskRef& ref);
ApprovalTask* asApprovalTask(const TaskRef& ref);

#endif
