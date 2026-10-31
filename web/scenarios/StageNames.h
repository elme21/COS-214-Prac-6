// TEMPORARY - stands in for a name on WorkStage (Member 1). WorkStage declares a private `name`
// member, but no constructor, setter or getter uses it, so the name passed to
// CodeDefinitionBuilder::addStage cannot be read back. Delete this when WorkStage exposes the name.
#ifndef WEB_STAGENAMES_H
#define WEB_STAGENAMES_H

#include "ScenarioSpec.h"

#include <map>
#include <string>
#include <vector>

class WorkComponent;

/// @brief Path of child indices from the root, e.g. {1, 0} = root->getChild(1)->getChild(0).
typedef std::vector<int> TreePath;

/// @brief Writes a tree path as a key.
/// @param path the path
/// @return "1.0" for {1, 0}; "" for the root
std::string pathKey(const TreePath& path);

/**
 * @brief Stage names keyed by TREE POSITION (temporary stand-in, see the file comment).
 *
 * Every instance is a deep clone of the definition, so the same position names the same stage in
 * all of them. The positions are found by matching each built WorkStage to the spec stage that has
 * the same depth and the same set of tasks beneath it. That stays correct even though
 * CodeDefinitionBuilder::addDecorator moves a decorated task to the end of its stage.
 */
class StageNames {
public:
    /// @brief Matches every stage of a built definition to its spec stage.
    /// @param spec the spec the definition was built from
    /// @param definitionRoot the root of the registered definition
    void learn(const ScenarioSpec& spec, WorkComponent* definitionRoot);
    /// @brief Gets the name of the stage at a position.
    /// @param path the stage's position
    /// @return the stage name, or "Stage " followed by the path key if the position is unknown
    std::string nameAt(const TreePath& path) const;

private:
    std::map<std::string, std::string> names;   ///< pathKey -> stage name
};

#endif
