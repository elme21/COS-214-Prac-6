//ConcreteBuilder - implements Builder steps
#ifndef CODEDEFINITIONBUILDER_H
#define CODEDEFINITIONBUILDER_H

#include "DefinitionBuilder.h"
#include "WorkflowDefinition.h"

#include <memory>
#include <string>

/**
 * @class CodeDefinitionBuilder
 * @brief ConcreteBuilder in the Builder design pattern.
 *
 * Implements the construction steps defined by DefinitionBuilder.
 * Builds a reusable WorkflowDefinition by adding stages, tasks,
 * assignment strategies, decorators, dependencies and approval chains.
 */
class CodeDefinitionBuilder : public DefinitionBuilder {
private:
    /** @brief Stores the workflow definition being constructed. */
    std::shared_ptr<WorkflowDefinition> definition;

public:
    /**
     * @brief Creates a builder for a named workflow.
     * @param name Name of the workflow to construct.
     */
    explicit CodeDefinitionBuilder(const std::string& name);

    /**
     * @brief Adds a stage to the workflow.
     * @param name Name of the new stage.
     * @param parent Name of the parent stage.
     * An empty parent indicates the root stage.
     */
    void addStage(const std::string& name, const std::string& parent) override;

    /**
     * @brief Adds a task to an existing stage.
     * @param name Name of the new task.
     * @param kind Type of task: manual, automated or approval.
     * @param stage Name of the stage containing the task.
     */
    void addTask(const std::string& name,
                 const std::string& kind,
                 const std::string& stage) override;

    /**
     * @brief Sets the assignment strategy for a task.
     * @param task Name of the task to configure.
     * @param strategy Pointer to the assignment strategy.
     */
    void setAssignmentStrategy(
        const std::string& task,
        AssignmentStrategy* strategy) override;

    /**
     * @brief Adds a decorator to a task.
     * @param task Name of the task to decorate.
     * @param decorator Name of the decorator to apply.
     */
    void addDecorator(const std::string& task, const std::string& decorator) override;

    /**
     * @brief Adds a dependency between two tasks.
     * @param from Name of the prerequisite task.
     * @param to Name of the task that depends on it.
     */
    void addDependency(const std::string& from, const std::string& to) override;

    /**
     * @brief Assigns an approval chain to an approval task.
     * @param task Name of the approval task.
     * @param handler Pointer to the first approval handler.
     */
    void setApprovalChain( const std::string& task, ApprovalHandler* handler) override;

    /**
     * @brief Returns the completed workflow definition.
     * @return Shared pointer to the constructed WorkflowDefinition.
     */
    std::shared_ptr<WorkflowDefinition>
        getDefinition() override;
};

#endif
