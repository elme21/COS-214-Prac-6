#include "DefinitionTree.h"

#include "WorkComponent.h"
#include "WorkComponentDecorator.h"
#include "AuditDecorator.h"
#include "SecureDecorator.h"
#include "ValidationDecorator.h"
#include "PriorityDecorator.h"
#include "WorkItem.h"
#include "WorkIterator.h"

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
}

TreeTask unwrapNode(WorkComponent* node) {
    TreeTask task = { node, nullptr, std::vector<std::string>() };
    WorkComponent* current = node;
    while (WorkComponentDecorator* d = dynamic_cast<WorkComponentDecorator*>(current)) {
        task.decorators.push_back(decoratorName(d));
        current = d->getWrappedComponent();   // real Decorator accessor
    }
    task.item = dynamic_cast<WorkItem*>(current);
    return task;
}

std::vector<TreeTask> definitionTasks(WorkComponent* root) {
    std::vector<TreeTask> result;
    if (root == nullptr) return result;
    std::unique_ptr<WorkIterator> it(root->createIterator());   // Iterator pattern
    if (!it) return result;
    it->reset();
    while (it->hasNext()) {
        WorkComponent* node = it->next();
        if (node->getState() == nullptr) continue;   // stages have no state
        TreeTask task = unwrapNode(node);
        if (task.item != nullptr) result.push_back(task);
    }
    return result;
}
