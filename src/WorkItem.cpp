#include "WorkItem.h"
#include "WorkCoordinator.h"
#include "WorkItemState.h"
#include "CreatedState.h"
#include "DepthFirstIterator.h"

#include <iostream>

WorkItem::WorkItem()
    : state(&CreatedState::instance()), assignee(nullptr) {}

WorkItem::WorkItem(const std::string& taskName, const std::string& taskId)
    : state(&CreatedState::instance()), assignee(nullptr), name(taskName), id(taskId) {}

WorkItem::~WorkItem() {}

// --- Identity ---

void WorkItem::setName(const std::string& taskName) { name = taskName; }
void WorkItem::setId(const std::string& taskId) { id = taskId; }
const std::string& WorkItem::getName() const { return name; }
const std::string& WorkItem::getId() const { return id; }

// --- Template Method ---

void WorkItem::execute() {
    if (!checkReady()) return;
    onEnter();
    bool finished = perform();
    recordResult(finished);
}

bool WorkItem::checkReady() const { return state->canExecute(); }

void WorkItem::onEnter() {
    std::cout << "[WorkItem] executing (state: " << state->name() << ")" << std::endl;
}

void WorkItem::recordResult(bool finished) {
    if (finished) complete();
    // otherwise stay InProgress; a later command finishes the item
}

// --- Composite (leaf) ---

void WorkItem::add(WorkComponent*) {}
void WorkItem::remove(WorkComponent*) {}
WorkComponent* WorkItem::getChild(int) { return nullptr; }

WorkIterator* WorkItem::createIterator() {
    // Member 1's DepthFirstIterator handles a single leaf: it visits the root,
    // and getChild() returns nullptr here, so nothing else is pushed.
    // Same contract as WorkStage::createIterator(): the caller owns the
    // iterator and must call reset() before the first hasNext()/next().
    return new DepthFirstIterator(this);
}

// --- State delegation ---

bool WorkItem::makeAvailable() { return state->makeAvailable(*this); }
bool WorkItem::assignTo(Participant* participant) { return state->assign(*this, participant); }
bool WorkItem::start() { return state->start(*this); }
bool WorkItem::approve() { return state->approve(*this); }
bool WorkItem::complete() { return state->complete(*this); }
bool WorkItem::reject() { return state->reject(*this); }
bool WorkItem::cancel() { return state->cancel(*this); }
bool WorkItem::escalate() { return state->escalate(*this); }

// --- Undo support ---

const WorkItemState* WorkItem::getState() const { return state; }
Participant* WorkItem::getAssignee() const { return assignee; }

void WorkItem::restoreState(const WorkItemState* restoredState, Participant* restoredAssignee) {
    state = restoredState;
    assignee = restoredAssignee;
}

void WorkItem::receive(const std::string&, Colleague*)
{
    if (coordinator == nullptr)
    {
        return;
    }

    if (coordinator->canBecomeAvailable(this))
    {
        makeAvailable();
    }
}

bool WorkItem::isComplete() const
{
    return state->isComplete();
}

// --- Private, reachable only by WorkItemState ---

void WorkItem::changeState(const WorkItemState* next) { state = next; }
void WorkItem::setAssignee(Participant* participant) { assignee = participant; }
