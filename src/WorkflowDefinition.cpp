
#include "../include/WorkflowDefinition.h"
#include "../include/WorkComponent.h"

#include <stdexcept>
#include <utility>

WorkflowDefinition::WorkflowDefinition(
    const std::string& name,
    std::shared_ptr<WorkComponent> root)
    : name(name),
      root(std::move(root)) {
}

const std::string& WorkflowDefinition::getName() const {
    return name;
}

std::shared_ptr<WorkComponent>
WorkflowDefinition::getRoot() const {
    return root;
}

std::shared_ptr<WorkComponent>
WorkflowDefinition::cloneRoot() const {

    if (!root) {
        throw std::runtime_error(
            "Cannot clone a workflow without a root");
    }

    return std::shared_ptr<WorkComponent>(
        root->clone());
}
