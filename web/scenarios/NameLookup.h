// TEMPORARY - stands in for stage names on WorkStage (Member 1) and for typed access to
// tasks through WorkComponent (shared contract, Members 1/3/5) until WorkStage carries a
// name and the tree can be searched by task ID without a cast. Delete when that lands.
#ifndef WEB_NAMELOOKUP_H
#define WEB_NAMELOOKUP_H

#include <map>
#include <string>
#include <vector>

class WorkComponent;
class WorkItem;
class AutomatedTask;
class ApprovalTask;

/** Path of child indices from the root, e.g. {1, 0} = root->getChild(1)->getChild(0). */
typedef std::vector<int> TreePath;

/** "1.0" for {1, 0}; "" for the root. */
std::string pathKey(const TreePath& path);

/**
 * Stage names keyed by TREE POSITION, not by pointer: every instance is a deep
 * clone with different pointers but the same shape, so one lookup serves the
 * definition and all of its instances.
 */
class NameLookup {
public:
    void setStageName(const TreePath& path, const std::string& name);
    std::string stageName(const TreePath& path) const;

private:
    std::map<std::string, std::string> stages;
};

/** Walks root->getChild(i)... along the path (Composite interface only). */
WorkComponent* componentAt(WorkComponent* root, const TreePath& path);

// WorkComponent has no getName()/getId() and no typed child accessors, so the web
// layer needs a cast to reach the Member 2 accessors on the leaves.
WorkItem* asWorkItem(WorkComponent* component);
AutomatedTask* asAutomatedTask(WorkComponent* component);
ApprovalTask* asApprovalTask(WorkComponent* component);

/** Finds a task by its definition-level ID using the tree's own iterator (createIterator). */
WorkItem* findTask(WorkComponent* root, const std::string& taskId);

/** All leaves in depth-first order, via createIterator(). */
std::vector<WorkItem*> allTasks(WorkComponent* root);

#endif
