// TEMPORARY - stands in for a name on WorkStage (Member 1) until WorkStage stores the name
// passed to CodeDefinitionBuilder::addStage. Delete when that lands and read the name instead.
#ifndef WEB_STAGENAMES_H
#define WEB_STAGENAMES_H

#include "ScenarioSpec.h"

#include <map>
#include <string>
#include <vector>

class WorkComponent;

/** Path of child indices from the root, e.g. {1, 0} = root->getChild(1)->getChild(0). */
typedef std::vector<int> TreePath;

/** "1.0" for {1, 0}; "" for the root. */
std::string pathKey(const TreePath& path);

/**
 * Stage names keyed by TREE POSITION. Every instance is a deep clone of the definition, so the
 * same position names the same stage in all of them.
 *
 * The positions are found by matching each built WorkStage to the spec stage that has the same
 * depth and the same set of tasks beneath it. That stays correct even though
 * CodeDefinitionBuilder::addDecorator moves a decorated task to the end of its stage.
 */
class StageNames {
public:
    void learn(const ScenarioSpec& spec, WorkComponent* definitionRoot);
    std::string nameAt(const TreePath& path) const;

private:
    std::map<std::string, std::string> names;   // pathKey -> stage name
};

#endif
