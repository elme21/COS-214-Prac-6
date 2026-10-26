#ifndef EVENT_H
#define EVENT_H

#include <string>
#include <vector>
#include <ctime>

class WorkItem;  // owned by Person 2; only a pointer is carried around

/** The kinds of things that can happen in the workflow engine. */
enum class EventType {
    INSTANCE_CREATED,
    INSTANCE_STARTED,
    ITEM_AVAILABLE,
    ITEM_ASSIGNED,
    ITEM_COMPLETED,
    ITEM_REJECTED,
    ITEM_CANCELLED,
    DEADLINE_MISSED,
    EXTERNAL_EVENT      ///< e.g. "payment received" (name goes in 'detail')
};

/** Readable name of an event type (table lookup, no switch). */
std::string eventTypeToString(EventType type);

/** Every event type, for listeners that want to hear about everything. */
const std::vector<EventType>& allEventTypes();

/** Plain value object describing one thing that happened. */
struct Event {
    EventType   type;
    std::string instanceId;
    std::string itemId;
    std::string detail;
    WorkItem*   item;        ///< may be nullptr
    std::time_t timestamp;

    Event(EventType t = EventType::EXTERNAL_EVENT,
          const std::string& instance = "",
          const std::string& itemIdentifier = "",
          const std::string& info = "",
          WorkItem* workItem = nullptr);
};

/** The 'State' of the class diagram: what WorkflowEventBus::getState() returns. */
typedef Event State;

#endif
