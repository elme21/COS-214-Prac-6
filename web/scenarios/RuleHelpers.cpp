#include "RuleHelpers.h"

#include "Trace.h"
#include "WorkItem.h"
#include "WorkItemState.h"
#include "ResourceCoordinator.h"

std::string stateOf(const WorkItem* item) {
    return item == nullptr || item->getState() == nullptr ? std::string("?") : item->getState()->name();
}

bool cancelByRule(RuleContext& context, const std::string& taskId, const std::string& why) {
    WorkItem* item = context.task(taskId);
    if (item == nullptr) return false;
    std::string before = stateOf(item);
    bool ok = item->cancel();
    if (ok) {
        context.trace().engine(item->getName() + ".cancel()", "State",
                               before + "State -> CancelledState (" + why + ")");
    } else {
        context.trace().engine(item->getName() + ".cancel()", "State",
                               before + "State refused cancel(); task left as " + before + " (" + why + ")");
    }
    return ok;
}

bool makeAvailableByRule(RuleContext& context, const std::string& taskId, const std::string& why) {
    WorkItem* item = context.task(taskId);
    if (item == nullptr) return false;
    std::string before = stateOf(item);
    bool ok = item->makeAvailable();
    context.trace().engine(item->getName() + ".makeAvailable()", "State",
                           ok ? before + "State -> AvailableState (" + why + ")"
                              : before + "State refused makeAvailable() (" + why + ")");
    return ok;
}

void releaseByRule(RuleContext& context, const std::string& taskId, const std::string& why) {
    WorkItem* item = context.task(taskId);
    if (item == nullptr) return;
    context.coordinator().release(item);
    context.data().erase("resource." + taskId);
    context.trace().engine("ResourceCoordinator::release(" + item->getName() + ")", "Mediator", why);
}

std::string valueOr(const CaseData& data, const std::string& key, const std::string& fallback) {
    CaseData::const_iterator it = data.find(key);
    return it == data.end() || it->second.empty() ? fallback : it->second;
}
