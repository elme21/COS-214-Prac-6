#ifndef MANUALTASK_H
#define MANUALTASK_H

#include "WorkItem.h"

/** A task a human does; it is finished later by a complete() command. */
class ManualTask : public WorkItem {
public:
    ManualTask();
    explicit ManualTask(const std::string& name, const std::string& id = std::string());

    // Prototype - coordinate with Member 3
    WorkComponent* clone() const override;

protected:
    bool perform() override;
};

#endif
