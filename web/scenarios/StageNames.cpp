// TEMPORARY - stands in for a name on WorkStage (Member 1) until WorkStage stores the name
// passed to CodeDefinitionBuilder::addStage. Delete when that lands and read the name instead.
#include "StageNames.h"

#include "TaskLookup.h"
#include "WorkComponent.h"
#include "WorkItem.h"

#include <algorithm>
#include <sstream>

std::string pathKey(const TreePath& path) {
    std::ostringstream out;
    for (size_t i = 0; i < path.size(); ++i) {
        if (i > 0) out << '.';
        out << path[i];
    }
    return out.str();
}

namespace {

std::string signature(int depth, std::vector<std::string> ids) {
    std::sort(ids.begin(), ids.end());
    std::ostringstream out;
    out << depth << '|';
    for (size_t i = 0; i < ids.size(); ++i) out << ids[i] << ',';
    return out.str();
}

std::vector<std::string> specTasks(const StageSpec& stage) {
    std::vector<std::string> ids;
    for (size_t i = 0; i < stage.tasks.size(); ++i) ids.push_back(stage.tasks[i].id);
    for (size_t i = 0; i < stage.stages.size(); ++i) {
        std::vector<std::string> sub = specTasks(stage.stages[i]);
        ids.insert(ids.end(), sub.begin(), sub.end());
    }
    return ids;
}

void indexSpec(const std::vector<StageSpec>& stages, int depth, std::map<std::string, std::string>& bySignature) {
    for (size_t i = 0; i < stages.size(); ++i) {
        bySignature[signature(depth, specTasks(stages[i]))] = stages[i].name;
        indexSpec(stages[i].stages, depth + 1, bySignature);
    }
}

// Returns the task ids under node and records the names of the stages it finds.
std::vector<std::string> walk(WorkComponent* node, const TreePath& path, int depth,
                              const std::map<std::string, std::string>& bySignature,
                              std::map<std::string, std::string>& names) {
    std::vector<std::string> ids;
    if (node->getState() != nullptr) {          // a task (possibly decorated)
        TaskRef ref = refFor(node);
        if (ref.item != nullptr) ids.push_back(ref.item->getId());
        return ids;
    }
    for (int i = 0; node->getChild(i) != nullptr; ++i) {
        TreePath child = path;
        child.push_back(i);
        std::vector<std::string> sub = walk(node->getChild(i), child, depth + 1, bySignature, names);
        ids.insert(ids.end(), sub.begin(), sub.end());
    }
    if (depth > 0) {
        std::map<std::string, std::string>::const_iterator it = bySignature.find(signature(depth, ids));
        if (it != bySignature.end()) names[pathKey(path)] = it->second;
    }
    return ids;
}

} // namespace

void StageNames::learn(const ScenarioSpec& spec, WorkComponent* root) {
    names.clear();
    std::map<std::string, std::string> bySignature;
    indexSpec(spec.stages, 1, bySignature);
    names[""] = spec.name;
    walk(root, TreePath(), 0, bySignature, names);
}

std::string StageNames::nameAt(const TreePath& path) const {
    std::map<std::string, std::string>::const_iterator it = names.find(pathKey(path));
    return it == names.end() ? "Stage " + pathKey(path) : it->second;
}
