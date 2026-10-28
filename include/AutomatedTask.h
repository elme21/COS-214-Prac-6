#ifndef AUTOMATEDTASK_H
#define AUTOMATEDTASK_H

#include "WorkItem.h"

#include <string>

/// @brief
/// The AutomatedTask class is a ConcreteClass of the Template Method pattern (and a Composite leaf):
/// an automated step that the system runs itself, with no person doing the work.
/// Its primitive operation perform() runs the step and returns whether it succeeded. On success,
/// recordResult() completes the item; on failure the item stays InProgress, so it can be escalated and
/// finished by hand.
class AutomatedTask : public WorkItem {
public:
    /// @brief The default constructor. No name or ID; the automated step succeeds.
    AutomatedTask();
    /// @brief The constructor. The automated step succeeds unless setStepSucceeds(false) is called.
    /// @param name the display name of the task
    /// @param id the ID of the task within its definition
    explicit AutomatedTask(const std::string& name,
                           const std::string& id = std::string());

    /// @brief sets whether the automated step succeeds when perform() runs it
    /// @param succeeds true if the step succeeds (the item completes), false if it fails (the item stays InProgress)
    void setStepSucceeds(bool succeeds);
    /// @brief gets whether the automated step succeeds when perform() runs it
    /// @return true if the step succeeds, false if it fails
    bool getStepSucceeds() const;

    // Prototype - coordinate with Member 3
    /// @brief creates a copy of this task (Prototype), including its state, assignee, name, ID and
    /// step outcome
    /// @return a new AutomatedTask owned by the caller
    WorkComponent* clone() const override;

protected:
    /// @brief the primitive operation: logs the automated step and reports its outcome
    /// @return stepSucceeds: true if the step succeeded (recordResult() then completes the item),
    /// false if it failed (the item stays InProgress)
    bool perform() override;

private:
    /// @brief Whether the automated step succeeds when perform() runs it (default true)
    bool stepSucceeds;
};

#endif
