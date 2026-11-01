#ifndef WEB_DEFINITIONTREE_H
#define WEB_DEFINITIONTREE_H

#include <string>
#include <vector>

class WorkComponent;
class WorkItem;

/**
 * @file DefinitionTree.h
 * @brief Reads a tree node for display: which WorkItem it is and which decorators wrap it.
 *
 * Cases are reached through WorkflowManager (getTree, getTask, getItem, getTaskIds). A definition is
 * built before any case exists and is outside the Facade, so the scenario tab and the builder step
 * read it with the Composite's own iterator and the Decorator's getWrappedComponent().
 */

/// @brief One task node of a tree, unwrapped.
struct TreeTask {
    WorkComponent* node;                   ///< The outermost component (a decorator if the task is decorated)
    WorkItem* item;                        ///< The WorkItem inside all decorators, or nullptr for a stage
    std::vector<std::string> decorators;   ///< Decorator class names, outermost first, e.g. {"AuditDecorator"}
};

/// @brief Unwraps one node with WorkComponentDecorator::getWrappedComponent().
/// @param node the node
/// @return the unwrapped task; item is nullptr for a stage
TreeTask unwrapNode(WorkComponent* node);

/// @brief Lists every task of a definition's tree with its own iterator (createIterator()).
/// @param root the definition's root
/// @return the tasks in depth-first order
std::vector<TreeTask> definitionTasks(WorkComponent* root);

#endif
