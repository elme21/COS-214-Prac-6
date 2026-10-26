#ifndef MANUALTASK_H
#define MANUALTASK_H

#include "WorkItem.h"

/** A task a human does; it is finished later by a complete() command. */
class ManualTask : public WorkItem {
public:
    ManualTask();

    // Prototype - coordinate with Member 3
    WorkComponent* clone() const override;

protected:
    bool perform() override;
};

#endif
