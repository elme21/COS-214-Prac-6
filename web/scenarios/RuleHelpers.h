#ifndef WEB_RULEHELPERS_H
#define WEB_RULEHELPERS_H

#include "Scenario.h"

#include <string>

class WorkItem;

/** Name of the item's current engine state ("Available", "Completed", ...). */
std::string stateOf(const WorkItem* item);

/** Calls the engine's cancel() on a task and records the result in the trace. */
bool cancelByRule(RuleContext& context, const std::string& taskId, const std::string& why);

/** Calls the engine's makeAvailable() on a task and records the result in the trace. */
bool makeAvailableByRule(RuleContext& context, const std::string& taskId, const std::string& why);

/** Calls ResourceCoordinator::release() for a task and records it in the trace. */
void releaseByRule(RuleContext& context, const std::string& taskId, const std::string& why);

/** Case-data lookup with a default. */
std::string valueOr(const CaseData& data, const std::string& key, const std::string& fallback);

#endif
