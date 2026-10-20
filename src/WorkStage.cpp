#include "WorkStage.h"
#include "DepthFirstIterator.h"

WorkStage::~WorkStage() 
{
    for (size_t i = 0; i < children.size(); ++i) 
    {
        delete children[i];
    }
}


void WorkStage::execute() 
{
    for (size_t i = 0; i < children.size(); ++i) 
    {
        children[i]->execute();
    }
}


void WorkStage::add(WorkComponent* component) 
{
    if (component != nullptr) {
        children.push_back(component);
    }
}


// Removes the pointer only; the caller keeps ownership.
void WorkStage::remove(WorkComponent* component) 
{
    for (size_t i = 0; i < children.size(); ++i) 
    {
        if (children[i] == component) 
        {
            children.erase(children.begin() + i);
            return;
        }
    }
}


// Returns nullptr for an out-of-range index
WorkComponent* WorkStage::getChild(int index) 
{
    if (index < 0 || static_cast<size_t>(index) >= children.size()) 
    {
        return nullptr;
    }
    return children[index];
}


WorkIterator* WorkStage::createIterator() 
{
    return new DepthFirstIterator(this);
}


WorkComponent* WorkStage::clone() const         //Prototype must double check 
{
    WorkStage* copy = new WorkStage();
    for (size_t i = 0; i < children.size(); ++i) 
    {
        copy->children.push_back(children[i]->clone());
    }
    return copy;
}

bool WorkStage::makeAvailable() { return false; }
bool WorkStage::assignTo(Participant*) { return false; }
bool WorkStage::start() { return false; }
bool WorkStage::approve() { return false; }
bool WorkStage::complete() { return false; }
bool WorkStage::reject() { return false; }
bool WorkStage::cancel() { return false; }
bool WorkStage::escalate() { return false; }

const WorkItemState* WorkStage::getState() const { return nullptr; }
Participant* WorkStage::getAssignee() const { return nullptr; }
void WorkStage::restoreState(const WorkItemState*, Participant*) {}