#include "RuleHelpers.h"

#include "Trace.h"
#include "WorkComponent.h"
#include "WorkItem.h"
#include "WorkItemState.h"
#include "ResourceCoordinator.h"
#include "CancelCommand.h"
#include "MakeAvailableCommand.h"

std::string stateOf(const WorkComponent* component) {
    return component == nullptr || component->getState() == nullptr ? std::string("?")
                                                                     : component->getState()->name();
}

bool cancelByRule(RuleContext& context, const std::string& taskId, const std::string& why) {
    TaskRef ref = context.task(taskId);
    if (ref.node == nullptr) return false;
    std::string before = stateOf(ref.node);
    bool ok = context.run(new CancelCommand(ref.node), "CancelCommand(" + taskId + ")");
    context.trace().engine("CancelCommand(" + ref.item->getName() + ") -> CommandProcessor::run", "Command + State",
                           ok ? before + "State -> CancelledState (" + why + ")"
                              : before + "State refused cancel(); task left as " + before + " (" + why + ")");
    return ok;
}

bool makeAvailableByRule(RuleContext& context, const std::string& taskId, const std::string& why) {
    TaskRef ref = context.task(taskId);
    if (ref.node == nullptr) return false;
    std::string before = stateOf(ref.node);
    bool ok = context.run(new MakeAvailableCommand(ref.node), "MakeAvailableCommand(" + taskId + ")");
    context.trace().engine("MakeAvailableCommand(" + ref.item->getName() + ") -> CommandProcessor::run",
                           "Command + State",
                           ok ? before + "State -> AvailableState (" + why + ")"
                              : before + "State refused makeAvailable() (" + why + ")");
    return ok;
}

void releaseByRule(RuleContext& context, const std::string& taskId, const std::string& why) {
    TaskRef ref = context.task(taskId);
    if (ref.item == nullptr) return;
    context.coordinator().release(ref.item);
    context.data().erase("resource." + taskId);
    context.data().erase("resourceBy." + taskId);
    context.trace().engine("ResourceCoordinator::release(" + ref.item->getName() + ")", "Mediator",
                           why + " (Resource::giveBack() frees the place)");
}

std::string valueOr(const CaseData& data, const std::string& key, const std::string& fallback) {
    CaseData::const_iterator it = data.find(key);
    return it == data.end() || it->second.empty() ? fallback : it->second;
}
