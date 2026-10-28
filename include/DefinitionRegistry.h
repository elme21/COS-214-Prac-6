/**
 * @file DefinitionRegistry.h
 * @brief Prototype manager holding workflow definitions and task prototypes.
 */
#ifndef DEFINITIONREGISTRY_H
#define DEFINITIONREGISTRY_H

#include <map>
#include <memory>
#include <string>
#include "WorkComponent.h"

class WorkflowDefinition;

/**
 * @brief Owns finished workflow definitions and prototypical work components.
 *
 * Instances are never built from scratch: callers clone a definition's root
 * (see WorkflowDefinition::cloneRoot()) or a task prototype, so every running
 * instance gets its own deep copy and one instance never affects another.
 *
 * @par Design pattern
 * Prototype (PrototypeManager role). The Client is WorkflowManager, which
 * creates instances; the Builder uses the task prototypes to create tasks by
 * kind without a switch statement.
 */
class DefinitionRegistry {
public:
    /** @brief Creates an empty registry. */
    DefinitionRegistry();

    /** @brief Destroys every definition and prototype it owns. */
    ~DefinitionRegistry();

    DefinitionRegistry(const DefinitionRegistry&) = delete;             ///< Not copyable.
    DefinitionRegistry& operator=(const DefinitionRegistry&) = delete;  ///< Not assignable.

    /**
     * @brief Registers a finished definition under its own name.
     * @param definition Definition to store. Ownership is taken even on failure.
     * @return false if @p definition is null or its name is already registered.
     */
    bool registerDefinition(WorkflowDefinition* definition);

    /**
     * @brief Looks up a definition by name.
     * @param name Name the definition was registered under.
     * @return The definition, or nullptr if unknown. The caller clones it with
     *         cloneRoot(); the registry keeps ownership.
     */
    const WorkflowDefinition* get(const std::string& name) const;

    /**
     * @brief Registers a prototype for a kind of work component.
     * @param kind Key, e.g. "manual" or "approval".
     * @param prototype Prototype to store. Ownership is taken even on failure.
     * @return false if @p prototype is null or @p kind is already registered.
     */
    bool addProto(const std::string& kind, WorkComponent* prototype);

    /**
     * @brief Removes and destroys the prototype for @p kind.
     * @param kind Key the prototype was registered under.
     * @return false if no prototype was registered for @p kind.
     */
    bool removeProto(const std::string& kind);

    /**
     * @brief Creates a new component by cloning the prototype for @p kind.
     * @param kind Key the prototype was registered under.
     * @return A deep copy owned by the caller, or nullptr if @p kind is unknown.
     */
    WorkComponent* cloneProto(const std::string& kind) const;

private:
    std::map<std::string, std::unique_ptr<WorkflowDefinition> > definitions;  ///< Owned, by name.
    std::map<std::string, std::unique_ptr<WorkComponent> > prototypes;        ///< Owned, by kind.
};

#endif