// TEMPORARY - stands in for task lookup by ID on WorkflowManager (Member 7) until the Facade
// exposes an instance's tasks. Delete when that lands.
#include "TaskLookup.h"

#include "WorkComponent.h"
#include "WorkComponentDecorator.h"
#include "AuditDecorator.h"
#include "SecureDecorator.h"
#include "ValidationDecorator.h"
#include "PriorityDecorator.h"
#include "WorkItem.h"
#include "WorkIterator.h"
#include "AutomatedTask.h"
#include "ApprovalTask.h"

#include <memory>

namespace {
// Display name of a concrete decorator (for the UI only; behaviour never depends on it).
std::string decoratorName(const WorkComponentDecorator* d) {
    if (dynamic_cast<const AuditDecorator*>(d)) return "AuditDecorator";
    if (dynamic_cast<const SecureDecorator*>(d)) return "SecureDecorator";
    if (dynamic_cast<const ValidationDecorator*>(d)) return "ValidationDecorator";
    if (dynamic_cast<const PriorityDecorator*>(d)) return "PriorityDecorator";
    return "WorkComponentDecorator";
}

TaskRef unwrap(WorkComponent* node) {
    TaskRef ref = { node, nullptr, std::vector<std::string>() };
    WorkComponent* current = node;
    while (WorkComponentDecorator* d = dynamic_cast<WorkComponentDecorator*>(current)) {
        ref.decorators.push_back(decoratorName(d));
        current = d->getWrappedComponent();   // real Decorator accessor
    }
    ref.item = dynamic_cast<WorkItem*>(current);
    return ref;
}
}

TaskRef refFor(WorkComponent* node) { return unwrap(node); }

std::vector<TaskRef> allTasks(WorkComponent* root) {
    std::vector<TaskRef> result;
    if (root == nullptr) return result;
    std::unique_ptr<WorkIterator> it(root->createIterator());   // Iterator pattern
    if (!it) return result;
    it->reset();
    while (it->hasNext()) {
        WorkComponent* node = it->next();
        if (node->getState() == nullptr) continue;   // stages have no state
        TaskRef ref = unwrap(node);
        if (ref.item != nullptr) result.push_back(ref);
    }
    return result;
}

TaskRef findTask(WorkComponent* root, const std::string& taskId) {
    std::vector<TaskRef> tasks = allTasks(root);
    for (size_t i = 0; i < tasks.size(); ++i) {
        if (tasks[i].item->getId() == taskId) return tasks[i];
    }
    TaskRef none = { nullptr, nullptr, std::vector<std::string>() };
    return none;
}

AutomatedTask* asAutomatedTask(const TaskRef& ref) { return dynamic_cast<AutomatedTask*>(ref.item); }

ApprovalTask* asApprovalTask(const TaskRef& ref) { return dynamic_cast<ApprovalTask*>(ref.item); }
