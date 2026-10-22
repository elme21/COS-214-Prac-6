#ifndef WORKSTAGE_H
#define WORKSTAGE_H

#include <vector>
#include "WorkComponent.h"

class WorkStage : public WorkComponent 
{
    private:
        std::vector<WorkComponent*> children;

    public:
        WorkStage() {}
        virtual ~WorkStage();

        // Composite / Template Method / Prototype
        void execute() override;
        void add(WorkComponent* component) override;
        void remove(WorkComponent* component) override;
        WorkComponent* getChild(int index) override;
        WorkIterator* createIterator() override;
        WorkComponent* clone() const override;

        // Lifecycle: a stage has no state of its own, so these are unsupported
        bool makeAvailable() override;
        bool assignTo(Participant* participant) override;
        bool start() override;
        bool approve() override;
        bool complete() override;
        bool reject() override;
        bool cancel() override;
        bool escalate() override;

        // Undo support
        const WorkItemState* getState() const override;
        Participant* getAssignee() const override;
        void restoreState(const WorkItemState* state, Participant* assignee) override;
};

#endif