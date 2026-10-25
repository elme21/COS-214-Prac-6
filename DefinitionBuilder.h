//Builder - defines the construction steps
#ifndef DEFINITIONBUILDER_H
#define DEFINITIONBUILDER_H

#include <string>
#include <memory>

class WorkflowDefinition;
class AssignmentStrategy;
class ApprovalHandler;

/**
 * @brief Abstract Builder interface for constructing workflow definitions.
 *
 * Defines the steps used to construct a workflow containing
 * stages, tasks, assignment strategies, decorators,
 * dependencies, and approval chains.
 *
 * Concrete builders implement these construction steps.
 */
class DefinitionBuilder {
public:
    virtual ~DefinitionBuilder() = default; //not on UML

    /**
     * @brief Adds a stage to the workflow.
     * @param name Name of the new stage.
     * @param parent Name of the parent stage.
     */
    virtual void addStage(const std::string& name,
                          const std::string& parent) = 0;

    /**
     * @brief Adds a task to an existing stage.
     * @param name Name of the task.
     * @param kind Type of task (manual, automated, approval).
     * @param stage Name of the stage containing the task.
     */
    virtual void addTask(const std::string& name,
                         const std::string& kind,
                         const std::string& stage) = 0;

    /**
     * @brief Sets the assignment strategy for a task.
     * @param task Name of the task.
     * @param strategy Strategy used to assign the task.
     */
    virtual void setAssignmentStrategy(
        const std::string& task,
        AssignmentStrategy* strategy) = 0;

    /**
     * @brief Adds a decorator to a task.
     * @param task Name of the task.
     * @param decorator Name of the decorator to apply.
     */
    virtual void addDecorator(const std::string& task,
                              const std::string& decorator) = 0;

    /**
     * @brief Creates a dependency between two tasks.
     * @param from Name of the prerequisite task.
     * @param to Name of the dependent task.
     */
    virtual void addDependency(const std::string& from,
                               const std::string& to) = 0;

    /**
     * @brief Assigns an approval chain to a task.
     * @param task Name of the approval task.
     * @param handler First handler in the approval chain.
     */
    virtual void setApprovalChain(
        const std::string& task,
        ApprovalHandler* handler) = 0;

    /**
     * @brief Returns the completed workflow definition.
     */
    virtual std::shared_ptr<WorkflowDefinition> //return type vs  UML
        getDefinition() = 0;
};

#endif
