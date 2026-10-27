#include "ReadyTaskIterator.h"
#include "WorkComponent.h"
#include "AvailableState.h"

ReadyTaskIterator::ReadyTaskIterator(WorkComponent* r) : root(r), forward(nullptr), current(nullptr) { reset(); }


//Depth-first walk that stops at the next ready leaf
void ReadyTaskIterator::advance() 
{
    forward = nullptr;
    while (!pending.empty()) 
    {
        WorkComponent* node = pending.top();
        pending.pop();

        if (node->getChild(0) != nullptr) 
        {
            int n = 0;
            while (node->getChild(n) != nullptr) 
            {
                n++;
            }
            for (int i = n - 1; i >= 0; i--) 
            {
                pending.push(node->getChild(i));
            }
        }else if (isReadyItem(node)) {
            forward = node;
            return;
        }
    }
}


bool ReadyTaskIterator::isReadyItem(WorkComponent* node) 
{
    const WorkItemState* state = node->getState();
    if (state == nullptr) 
    {
        return false;   //Stages have no state
    }
    return dynamic_cast<const AvailableState*>(state) != nullptr;   //NOTE: to be changed if isAvailable is added to WorkItemState
}


void ReadyTaskIterator::reset() 
{
    while (!pending.empty()) 
    {
        pending.pop();
    }

    current = nullptr;
    forward = nullptr;
    if (root != nullptr) 
    {
        pending.push(root);
    }
    advance();
}


bool ReadyTaskIterator::hasNext() 
{
    return forward != nullptr;
}


WorkComponent* ReadyTaskIterator::next() 
{
    current = forward;
    advance();
    return current;
}


WorkComponent* ReadyTaskIterator::currentItem() 
{
    return current;
}