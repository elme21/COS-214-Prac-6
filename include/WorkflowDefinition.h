//Product -	Represents the finished workflow
#ifndef WORKFLOWDEFINITION_H
#define WORKFLOWDEFINITION_H

#include <string>
#include <memory>
#include <vector>

class WorkComponent;
class AssignmentStrategy;
class Dependency;

/**
 * @brief Product of the Builder pattern.
 *
 * Stores the reusable structure of a workflow.
 * Workflow instances will clone this definition's root
 * so that each instance can execute independently.
 */
class WorkflowDefinition {
private:
    std::string name;
    std::shared_ptr<WorkComponent> root;
    std::vector<std::shared_ptr<AssignmentStrategy>> strategies;
    std::vector<std::shared_ptr<Dependency>> dependencies;

public:
   /**
     * @brief Constructs a workflow definition.
     */
    WorkflowDefinition(
        const std::string& name,
        std::shared_ptr<WorkComponent> root
    );

    /**
     * @brief Returns the workflow definition name.
     */
    const std::string& getName() const;

    /**
     * @brief Returns the root of the workflow structure.
     */
    std::shared_ptr<WorkComponent> getRoot() const;

    /**
     * @brief Creates an independent deep copy of the root.
     *
     * Uses the Prototype pattern implemented by WorkComponent.
     */
    std::shared_ptr<WorkComponent> cloneRoot() const; 
};

#endif
