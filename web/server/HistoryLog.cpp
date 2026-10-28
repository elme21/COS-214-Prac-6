// TEMPORARY - stands in for a readable command history on CommandProcessor (Member 5) and
// for AuditLogger (Observer, Member 6) until CommandProcessor exposes its history and
// AuditLogger records events. Delete when that lands.
#include "HistoryLog.h"

#include <ctime>

namespace {
std::string clockTime() {
    std::time_t now = std::time(nullptr);
    char buffer[16];
    std::strftime(buffer, sizeof(buffer), "%H:%M:%S", std::localtime(&now));
    return buffer;
}
}

HistoryEntry& HistoryLog::add(const std::string& action, const std::string& taskId, const std::string& taskName,
                              const std::string& detail, const std::string& result) {
    HistoryEntry entry;
    entry.sequence = next++;
    entry.time = clockTime();
    entry.action = action;
    entry.taskId = taskId;
    entry.taskName = taskName;
    entry.detail = detail;
    entry.result = result;
    entry.onCommandStack = false;
    items.push_back(entry);
    return items.back();
}

HistoryEntry* HistoryLog::markLatestUndone() {
    for (size_t i = items.size(); i > 0; --i) {
        if (items[i - 1].onCommandStack) {
            items[i - 1].onCommandStack = false;
            items[i - 1].result = "undone";
            return &items[i - 1];
        }
    }
    return nullptr;
}

void HistoryLog::clearCommandStack() {
    for (size_t i = 0; i < items.size(); ++i) items[i].onCommandStack = false;
}
