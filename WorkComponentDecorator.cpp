#include "WorkComponentDecorator.h"
#include <stdexcept>

WorkComponentDecorator::WorkComponentDecorator(WorkComponent* component)
    : wrapped(component) {
    if (!wrapped) {
        throw std::invalid_argument("WorkComponentDecorator: component must not be null");
    }
}

WorkComponentDecorator::WorkComponentDecorator(const WorkComponentDecorator& other)
    : WorkComponent(other), wrapped(other.wrapped->clone()) {}

WorkComponentDecorator::~WorkComponentDecorator() {}

void WorkComponentDecorator::execute() { wrapped->execute(); }
void WorkComponentDecorator::add(WorkComponent* child) { wrapped->add(child); }
void WorkComponentDecorator::remove(WorkComponent* child) { wrapped->remove(child); }
WorkComponent* WorkComponentDecorator::getChild(int index) { return wrapped->getChild(index); }
WorkIterator* WorkComponentDecorator::createIterator() { return wrapped->createIterator(); }

bool WorkComponentDecorator::makeAvailable() { return wrapped->makeAvailable(); }
bool WorkComponentDecorator::assignTo(Participant* p) { return wrapped->assignTo(p); }
bool WorkComponentDecorator::start() { return wrapped->start(); }
bool WorkComponentDecorator::approve() { return wrapped->approve(); }
bool WorkComponentDecorator::complete() { return wrapped->complete(); }
bool WorkComponentDecorator::reject() { return wrapped->reject(); }
bool WorkComponentDecorator::cancel() { return wrapped->cancel(); }
bool WorkComponentDecorator::escalate() { return wrapped->escalate(); }

int WorkComponentDecorator::getPriority() const { return wrapped->getPriority(); }

WorkItemState* WorkComponentDecorator::getState() const { return wrapped->getState(); }
Participant* WorkComponentDecorator::getAssignee() const { return wrapped->getAssignee(); }
void WorkComponentDecorator::restoreState(WorkItemState* state, Participant* assignee) {
    wrapped->restoreState(state, assignee);
}