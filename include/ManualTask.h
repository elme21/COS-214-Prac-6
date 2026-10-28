#ifndef MANUALTASK_H
#define MANUALTASK_H

#include "WorkItem.h"

/// @brief
/// The ManualTask class is a ConcreteClass of the Template Method pattern (and a Composite leaf):
/// a task a human does; it is finished later by a complete() command.
class ManualTask : public WorkItem {
public:
    /// @brief The default constructor. Starts in CreatedState with no name or ID.
    ManualTask();
    /// @brief The constructor
    /// @param name the display name of the task
    /// @param id the ID of the task within its definition
    explicit ManualTask(const std::string& name, const std::string& id = std::string());

    // Prototype - coordinate with Member 3
    /// @brief creates a copy of this task (Prototype), including its state, assignee, name and ID
    /// @return a new ManualTask owned by the caller
    WorkComponent* clone() const override;

protected:
    /// @brief the primitive operation: prints that the task is waiting for its assignee.
    /// @return always false, because a person finishes the work later with complete()
    bool perform() override;
};

#endif
