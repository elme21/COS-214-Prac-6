
#ifndef CODEDEFINITIONBUILDER_H
#define CODEDEFINITIONBUILDER_H

#include "DefinitionBuilder.h"
#include "WorkflowDefinition.h"

#include <memory>
#include <string>

/**
 * @brief ConcreteBuilder in the Builder design pattern.
 *
 * Implements the construction operations declared
 * by DefinitionBuilder to assemble a WorkflowDefinition.
 */
class CodeDefinitionBuilder : public DefinitionBuilder {
private:
    std::shared_ptr<WorkflowDefinition> definition;

public:
    /**
     * @brief Creates a builder for a named workflow.
     */
    explicit CodeDefinitionBuilder(const std::string& name);

    void addStage(const std::string& name,
                  const std::string& parent) override;

    void addTask(const std::string& name,
                 const std::string& kind,
                 const std::string& stage) override;

    void setAssignmentStrategy(
        const std::string& task,
        AssignmentStrategy* strategy) override;

    void addDecorator(const std::string& task,
                      const std::string& decorator) override;

    void addDependency(const std::string& from,
                       const std::string& to) override;

    void setApprovalChain(const std::string& task,
                          ApprovalHandler* handler) override;

    std::shared_ptr<WorkflowDefinition>
        getDefinition() override;
};

#endif
