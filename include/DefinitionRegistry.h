
#ifndef DEFINITIONREGISTRY_H
#define DEFINITIONREGISTRY_H

#include <map>
#include <memory>
#include <string>
#include "WorkComponent.h"

class WorkflowDefinition;

/// @brief Manages workflow definitions and prototype work components.
/// @details Owns registered definitions and prototypes. Callers create
/// independent instances by cloning a definition's root or a prototype.
class DefinitionRegistry {
public:
    /// @brief Constructs a definition registry.
    DefinitionRegistry();

    /// @brief Destroys the registry and its owned objects.
    ~DefinitionRegistry();

    DefinitionRegistry(const DefinitionRegistry&) = delete;
    DefinitionRegistry& operator=(const DefinitionRegistry&) = delete;

    /// @brief Registers a workflow definition by name.
    /// @param definition Pointer to the definition being registered.
    /// @return True if registered, false if the name already exists.
    /// @note The registry takes ownership even if registration fails.
    bool registerDefinition(WorkflowDefinition* definition);

    /// @brief Retrieves a registered workflow definition.
    /// @param name Name of the workflow definition.
    /// @return Pointer to the definition, or nullptr if not found.
    /// @note Callers can create independent instances using cloneRoot().
    const WorkflowDefinition* get(const std::string& name) const;

    /// @brief Registers a prototype work component by kind.
    /// @param kind Identifier for the prototype.
    /// @param prototype Pointer to the prototype being registered.
    /// @return True if registered, false if the kind already exists.
    /// @note The registry takes ownership even if registration fails.
    bool addProto(const std::string& kind, WorkComponent* prototype);

    /// @brief Removes a registered prototype.
    /// @param kind Identifier of the prototype to remove.
    /// @return True if removed, false if the kind was not found.
    bool removeProto(const std::string& kind);

    /// @brief Creates a deep copy of a registered prototype.
    /// @param kind Identifier of the prototype to clone.
    /// @return Pointer to a new copy, or nullptr if not found.
    /// @note The caller owns the returned copy.
    WorkComponent* cloneProto(const std::string& kind) const;

private:
    std::map<std::string, std::unique_ptr<WorkflowDefinition> > definitions;
    std::map<std::string, std::unique_ptr<WorkComponent> > prototypes;
};

#endif