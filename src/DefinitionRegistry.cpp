/**
 * @file DefinitionRegistry.cpp
 * @brief Implementation of DefinitionRegistry.
 */
#include "DefinitionRegistry.h"
#include "WorkflowDefinition.h"
#include <utility>

DefinitionRegistry::DefinitionRegistry() {}
DefinitionRegistry::~DefinitionRegistry() {}

bool DefinitionRegistry::registerDefinition(WorkflowDefinition* definition) {
    std::unique_ptr<WorkflowDefinition> owned(definition);
    if (!owned || definitions.count(owned->getName()) > 0) {
        return false;
    }
    const std::string name = owned->getName();
    definitions[name] = std::move(owned);
    return true;
}

const WorkflowDefinition* DefinitionRegistry::get(const std::string& name) const {
    std::map<std::string, std::unique_ptr<WorkflowDefinition> >::const_iterator it =
        definitions.find(name);
    return it == definitions.end() ? nullptr : it->second.get();
}

bool DefinitionRegistry::addProto(const std::string& kind, WorkComponent* prototype) {
    std::unique_ptr<WorkComponent> owned(prototype);
    if (!owned || prototypes.count(kind) > 0) {
        return false;
    }
    prototypes[kind] = std::move(owned);
    return true;
}

bool DefinitionRegistry::removeProto(const std::string& kind) {
    return prototypes.erase(kind) > 0;
}

WorkComponent* DefinitionRegistry::cloneProto(const std::string& kind) const {
    std::map<std::string, std::unique_ptr<WorkComponent> >::const_iterator it =
        prototypes.find(kind);
    return it == prototypes.end() ? nullptr : it->second->clone();
}
