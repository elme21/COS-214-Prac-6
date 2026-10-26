#include "Event.h"
#include <map>

namespace {
const std::map<EventType, std::string>& names() {
    static const std::map<EventType, std::string> table = {
        {EventType::INSTANCE_CREATED, "INSTANCE_CREATED"},
        {EventType::INSTANCE_STARTED, "INSTANCE_STARTED"},
        {EventType::ITEM_AVAILABLE,   "ITEM_AVAILABLE"},
        {EventType::ITEM_ASSIGNED,    "ITEM_ASSIGNED"},
        {EventType::ITEM_COMPLETED,   "ITEM_COMPLETED"},
        {EventType::ITEM_REJECTED,    "ITEM_REJECTED"},
        {EventType::ITEM_CANCELLED,   "ITEM_CANCELLED"},
        {EventType::DEADLINE_MISSED,  "DEADLINE_MISSED"},
        {EventType::EXTERNAL_EVENT,   "EXTERNAL_EVENT"}
    };
    return table;
}
}

std::string eventTypeToString(EventType type) {
    std::map<EventType, std::string>::const_iterator it = names().find(type);
    return it == names().end() ? "UNKNOWN" : it->second;
}

const std::vector<EventType>& allEventTypes() {
    static std::vector<EventType> all;
    if (all.empty()) {
        for (std::map<EventType, std::string>::const_iterator it = names().begin();
             it != names().end(); ++it) {
            all.push_back(it->first);
        }
    }
    return all;
}

Event::Event(EventType t, const std::string& instance, const std::string& itemIdentifier,
             const std::string& info, WorkItem* workItem)
    : type(t), instanceId(instance), itemId(itemIdentifier), detail(info),
      item(workItem), timestamp(std::time(nullptr)) {}