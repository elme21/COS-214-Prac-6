// TEMPORARY - stands in for stage names on WorkStage (Member 1) and for typed access to
// tasks through WorkComponent (shared contract, Members 1/3/5) until WorkStage carries a
// name and the tree can be searched by task ID without a cast. Delete when that lands.
#include "NameLookup.h"

#include "WorkComponent.h"
#include "WorkItem.h"
#include "WorkIterator.h"
#include "AutomatedTask.h"
#include "ApprovalTask.h"

#include <memory>
#include <sstream>

std::string pathKey(const TreePath& path) {
    std::ostringstream out;
    for (size_t i = 0; i < path.size(); ++i) {
        if (i > 0) out << '.';
        out << path[i];
    }
    return out.str();
}

void NameLookup::setStageName(const TreePath& path, const std::string& name) {
    stages[pathKey(path)] = name;
}

std::string NameLookup::stageName(const TreePath& path) const {
    std::map<std::string, std::string>::const_iterator it = stages.find(pathKey(path));
    return it == stages.end() ? std::string("Stage ") + pathKey(path) : it->second;
}

WorkComponent* componentAt(WorkComponent* root, const TreePath& path) {
    WorkComponent* node = root;
    for (size_t i = 0; node != nullptr && i < path.size(); ++i) {
        node = node->getChild(path[i]);
    }
    return node;
}

WorkItem* asWorkItem(WorkComponent* component) { return dynamic_cast<WorkItem*>(component); }

AutomatedTask* asAutomatedTask(WorkComponent* component) { return dynamic_cast<AutomatedTask*>(component); }

ApprovalTask* asApprovalTask(WorkComponent* component) { return dynamic_cast<ApprovalTask*>(component); }

std::vector<WorkItem*> allTasks(WorkComponent* root) {
    std::vector<WorkItem*> result;
    if (root == nullptr) return result;
    // Iterator pattern: the aggregate creates the iterator; Member 1's
    // DepthFirstIterator needs reset() before the first hasNext().
    std::unique_ptr<WorkIterator> it(root->createIterator());
    if (!it) return result;
    it->reset();
    while (it->hasNext()) {
        WorkItem* item = asWorkItem(it->next());
        if (item != nullptr) result.push_back(item);
    }
    return result;
}

WorkItem* findTask(WorkComponent* root, const std::string& taskId) {
    std::vector<WorkItem*> tasks = allTasks(root);
    for (size_t i = 0; i < tasks.size(); ++i) {
        if (tasks[i]->getId() == taskId) return tasks[i];
    }
    return nullptr;
}
