#ifndef WORKCOMPONENTDECORATOR_H
#define WORKCOMPONENTDECORATOR_H

#include <memory>
#include "WorkComponent.h"

// Abstract base: forwards every operation to the wrapped component and owns it.
// Concrete decorators override only the operations they hook, plus clone().
class WorkComponentDecorator : public WorkComponent {
public:
    explicit WorkComponentDecorator(WorkComponent* component);
    WorkComponentDecorator(const WorkComponentDecorator& other);
    WorkComponentDecorator& operator=(const WorkComponentDecorator&) = delete;
    virtual ~WorkComponentDecorator();

    void execute() override;
    void add(WorkComponent* child) override;
    void remove(WorkComponent* child) override;
    WorkComponent* getChild(int index) override;
    WorkIterator* createIterator() override;

    bool makeAvailable() override;
    bool assignTo(Participant* participant) override;
    bool start() override;
    bool approve() override;
    bool complete() override;
    bool reject() override;
    bool cancel() override;
    bool escalate() override;

    int getPriority() const override;

    WorkItemState* getState() const override;
    Participant* getAssignee() const override;
    void restoreState(WorkItemState* state, Participant* assignee) override;

    // clone() stays pure: each concrete decorator copies itself.

protected:
    WorkComponent* inner() const { return wrapped.get(); }

private:
    std::unique_ptr<WorkComponent> wrapped;
};

#endif