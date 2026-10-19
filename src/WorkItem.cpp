#include "WorkItem.h"
#include "WorkItemState.h"
#include "CreatedState.h"

#include <iostream>

WorkItem::WorkItem()
    : state(&CreatedState::instance()), assignee(nullptr), coordinator(nullptr) {}

WorkItem::~WorkItem() {}

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
    // TODO leaf iterator - confirm with Member 1
    return nullptr;
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

void WorkItem::setCoordinator(WorkCoordinator* newCoordinator) { coordinator = newCoordinator; }

// --- Private, reachable only by WorkItemState ---

void WorkItem::changeState(const WorkItemState* next) { state = next; }
void WorkItem::setAssignee(Participant* participant) { assignee = participant; }
