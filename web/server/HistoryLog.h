// TEMPORARY - stands in for a readable command history on CommandProcessor (Member 5) and
// for AuditLogger (Observer, Member 6) until CommandProcessor exposes its history and
// AuditLogger records events. Delete when that lands.
#ifndef WEB_HISTORYLOG_H
#define WEB_HISTORYLOG_H

#include <string>
#include <vector>

struct HistoryEntry {
    int sequence;
    std::string time;          // HH:MM:SS
    std::string action;        // assign, complete, undo, snapshot, ...
    std::string taskId;
    std::string taskName;
    std::string detail;
    std::string result;        // done | refused | undone | info
    std::vector<std::string> changes;
    bool onCommandStack;       // true while the matching command sits in CommandProcessor
};

/**
 * Per-instance audit trail. Every attempt is recorded, including refused ones,
 * which is what the spec's "invalid transition handling" needs to show.
 */
class HistoryLog {
public:
    HistoryLog() : next(1) {}

    HistoryEntry& add(const std::string& action, const std::string& taskId, const std::string& taskName,
                      const std::string& detail, const std::string& result);

    /** Marks the newest entry whose command is still on the CommandProcessor stack as undone. */
    HistoryEntry* markLatestUndone();

    /** After a snapshot restore the CommandProcessor is replaced, so nothing is undoable. */
    void clearCommandStack();

    const std::vector<HistoryEntry>& entries() const { return items; }

private:
    int next;
    std::vector<HistoryEntry> items;
};

#endif
