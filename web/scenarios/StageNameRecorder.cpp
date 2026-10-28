// TEMPORARY - stands in for a name on WorkStage (Member 1) until WorkStage stores the name
// passed to CodeDefinitionBuilder::addStage. Delete when that lands.
#include "StageNameRecorder.h"

#include <sstream>

std::string pathKey(const TreePath& path) {
    std::ostringstream out;
    for (size_t i = 0; i < path.size(); ++i) {
        if (i > 0) out << '.';
        out << path[i];
    }
    return out.str();
}

StageNameRecorder::StageNameRecorder(DefinitionBuilder& builder) : real(builder) {}

TreePath StageNameRecorder::nextChildPath(const std::string& parent) {
    TreePath path = stagePaths[parent];
    path.push_back(childCounts[parent]++);
    return path;
}

void StageNameRecorder::addStage(const std::string& name, const std::string& parent) {
    real.addStage(name, parent);
    TreePath path = parent.empty() ? TreePath() : nextChildPath(parent);
    stagePaths[name] = path;
    namesByPath[pathKey(path)] = name;
}

void StageNameRecorder::addTask(const std::string& name, const std::string& kind, const std::string& stage) {
    real.addTask(name, kind, stage);
    nextChildPath(stage);   // tasks take a child position too
}

void StageNameRecorder::setAssignmentStrategy(const std::string& task, AssignmentStrategy* strategy) {
    real.setAssignmentStrategy(task, strategy);
}

void StageNameRecorder::addDecorator(const std::string& task, const std::string& decorator) {
    real.addDecorator(task, decorator);
}

void StageNameRecorder::addDependency(const std::string& from, const std::string& to) {
    real.addDependency(from, to);
}

void StageNameRecorder::setApprovalChain(const std::string& task, ApprovalHandler* handler) {
    real.setApprovalChain(task, handler);
}

std::shared_ptr<WorkflowDefinition> StageNameRecorder::getDefinition() {
    return real.getDefinition();
}

std::string StageNameRecorder::stageName(const TreePath& path) const {
    std::map<std::string, std::string>::const_iterator it = namesByPath.find(pathKey(path));
    return it == namesByPath.end() ? "Stage " + pathKey(path) : it->second;
}
