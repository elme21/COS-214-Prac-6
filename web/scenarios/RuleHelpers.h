#ifndef WEB_RULEHELPERS_H
#define WEB_RULEHELPERS_H

#include "Scenario.h"

#include <string>

class WorkComponent;

/** Name of the component's current engine state ("Available", "Completed", ...). */
std::string stateOf(const WorkComponent* component);

/** Cancels a task with the real CancelCommand (through the CommandProcessor) and traces it. */
bool cancelByRule(RuleContext& context, const std::string& taskId, const std::string& why);

/** Makes a task available with the real MakeAvailableCommand and traces it. */
bool makeAvailableByRule(RuleContext& context, const std::string& taskId, const std::string& why);

/** Hands back a task's slot with the real ResourceCoordinator::release() and traces it. */
void releaseByRule(RuleContext& context, const std::string& taskId, const std::string& why);

/** Case-data lookup with a default. */
std::string valueOr(const CaseData& data, const std::string& key, const std::string& fallback);

#endif
