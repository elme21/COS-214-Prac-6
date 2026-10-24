
#ifndef DEFINITIONREGISTRY_H
#define DEFINITIONREGISTRY_H

#include <string>
#include <vector>
#include <memory>

class WorkflowDefinition;
class WorkComponent;

/**
 * @brief Registry for reusable workflow definitions.
 *
 * Stores completed WorkflowDefinition objects and
 * prototype WorkComponents for future cloning.
 */
class DefinitionRegistry {
private:
    std::vector<WorkComponent*> prototypeStore;

    std::vector<std::shared_ptr<WorkflowDefinition>> definitions;

public:
    /**
     * @brief Registers a completed workflow definition.
     */
    void registerDefinition(
        std::shared_ptr<WorkflowDefinition> definition);

    /**
     * @brief Retrieves a workflow definition by name.
     */
    std::shared_ptr<WorkflowDefinition>
        get(const std::string& name) const;

    /**
     * @brief Adds a prototype component.
     */
    void addProto(WorkComponent* w);

    /**
     * @brief Removes a prototype component.
     */
    void removeProto(WorkComponent* w);

    /**
     * @brief Retrieves a prototype component.
     */
    WorkComponent* getProto() const;
};

#endif
