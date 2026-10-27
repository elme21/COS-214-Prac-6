//Product -	Represents the finished workflow
#ifndef WORKFLOWDEFINITION_H
#define WORKFLOWDEFINITION_H

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <utility>

class WorkComponent;
class AssignmentStrategy;
class Dependency;
class ApprovalHandler;

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
    // Store task configuration by name so it can be applied to cloned instances.
    std::map<std::string, AssignmentStrategy*> strategies;
    std::vector<std::pair<std::string, std::string>> dependencies;

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

    /**
     * @brief Records a prerequisite and its dependent task.
     */
    void addDependency(const std::string& prerequisite, const std::string& dependent);

    /**
     * @brief Returns the dependency specifications for this definition.
     */
    const std::vector<std::pair<std::string, std::string>>& getDependencies() const;

    /**
     * @brief Associates a task with an assignment strategy.
     */
    void setStrategy(const std::string& task, AssignmentStrategy* strategy);

    /**
     * @brief Returns the configured assignment strategy for a task.
     */
    AssignmentStrategy* getStrategy(const std::string& task) const;

    /**
     * @brief Records an approval handler association (non-owning).
     */
    void keepApprovalHandler(ApprovalHandler* handler); 
};

#endif
