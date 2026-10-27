#ifndef DEFINITIONREGISTRY_H
#define DEFINITIONREGISTRY_H

#include <map>
#include <memory>
#include <string>
#include "WorkComponent.h"

class WorkflowDefinition;

// Prototype manager. Owns finished workflow definitions (by name) and prototypical work components (by kind)
//Instances are never built from scratch: callers clone the definition's root or a task prototype
class DefinitionRegistry {
public:
    DefinitionRegistry();
    ~DefinitionRegistry();
    DefinitionRegistry(const DefinitionRegistry&) = delete;
    DefinitionRegistry& operator=(const DefinitionRegistry&) = delete;

    // Takes ownership even when it returns false (name already registered).
    bool registerDefinition(WorkflowDefinition* definition);
    // nullptr if unknown. The caller clones via definition->cloneRoot().
    const WorkflowDefinition* get(const std::string& name) const;

    // Takes ownership even when it returns false (kind already registered).
    bool addProto(const std::string& kind, WorkComponent* prototype);
    bool removeProto(const std::string& kind);
    // A fresh deep copy owned by the caller, or nullptr if the kind is unknown.
    WorkComponent* cloneProto(const std::string& kind) const;

private:
    std::map<std::string, std::unique_ptr<WorkflowDefinition> > definitions;
    std::map<std::string, std::unique_ptr<WorkComponent> > prototypes;
};

#endif
