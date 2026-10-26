#include "NotificationSender.h"
#include "WorkflowEventBus.h"
#include <map>

namespace {
/** Event types the applicant hears about, and the opening words of each message. */
const std::map<EventType, std::string>& messageTable() {
    static const std::map<EventType, std::string> table = {
        {EventType::ITEM_ASSIGNED,   "A task has been assigned"},
        {EventType::ITEM_COMPLETED,  "A step has been completed"},
        {EventType::ITEM_REJECTED,   "A step was rejected"},
        {EventType::DEADLINE_MISSED, "A deadline was missed"},
        {EventType::EXTERNAL_EVENT,  "There is an update"}
    };
    return table;
}
}

NotificationSender::NotificationSender(WorkflowEventBus* s) : subject(s) {
    if (subject) {
        for (std::map<EventType, std::string>::const_iterator it = messageTable().begin();
             it != messageTable().end(); ++it) {
            subject->subscribe(it->first, this);
        }
    }
}

NotificationSender::~NotificationSender() {
    if (subject) {
        for (std::map<EventType, std::string>::const_iterator it = messageTable().begin();
             it != messageTable().end(); ++it) {
            subject->unsubscribe(it->first, this);
        }
    }
}

void NotificationSender::update() {
    const State* s = subject ? subject->getState() : nullptr;
    if (!s) return;

    std::map<EventType, std::string>::const_iterator it = messageTable().find(s->type);
    if (it == messageTable().end()) return;

    std::string msg = it->second + " (application " + s->instanceId;
    if (!s->itemId.empty()) msg += ", step " + s->itemId;
    msg += ")";
    if (!s->detail.empty()) msg += ": " + s->detail;
    sent.push_back(msg + ".");
}

const std::vector<std::string>& NotificationSender::getSent() const { return sent; }