// TEMPORARY - stands in for a name on WorkStage (Member 1) until WorkStage stores the name
// passed to CodeDefinitionBuilder::addStage. Delete when that lands.
#ifndef WEB_STAGENAMERECORDER_H
#define WEB_STAGENAMERECORDER_H

#include "DefinitionBuilder.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

/** Path of child indices from the root, e.g. {1, 0} = root->getChild(1)->getChild(0). */
typedef std::vector<int> TreePath;

/** "1.0" for {1, 0}; "" for the root. */
std::string pathKey(const TreePath& path);

/**
 * Forwards EVERY builder step unchanged to the real builder it wraps (normally a
 * CodeDefinitionBuilder). The only thing it adds is a note of each stage's name, keyed by
 * TREE POSITION, because WorkStage has no name and every instance is a deep clone with
 * different pointers but the same shape.
 *
 * Positions are counted per parent in the order stages and tasks are added. That stays
 * correct as long as a decorated task is not followed by a sub-stage in the same parent
 * (CodeDefinitionBuilder::addDecorator moves the decorated task to the end of its stage).
 */
class StageNameRecorder : public DefinitionBuilder {
public:
    explicit StageNameRecorder(DefinitionBuilder& real);

    void addStage(const std::string& name, const std::string& parent) override;
    void addTask(const std::string& name, const std::string& kind, const std::string& stage) override;
    void setAssignmentStrategy(const std::string& task, AssignmentStrategy* strategy) override;
    void addDecorator(const std::string& task, const std::string& decorator) override;
    void addDependency(const std::string& from, const std::string& to) override;
    void setApprovalChain(const std::string& task, ApprovalHandler* handler) override;
    std::shared_ptr<WorkflowDefinition> getDefinition() override;

    /** Name of the stage at this tree position (or "Stage <path>" if unknown). */
    std::string stageName(const TreePath& path) const;

private:
    TreePath nextChildPath(const std::string& parent);

    DefinitionBuilder& real;
    std::map<std::string, TreePath> stagePaths;
    std::map<std::string, int> childCounts;
    std::map<std::string, std::string> namesByPath;
};

#endif
