#include "ReadyTaskIterator.h"
#include "WorkComponent.h"
#include "WorkItemState.h"  

ReadyTaskIterator::ReadyTaskIterator(WorkComponent* r) : root(r), forward(nullptr), current(nullptr) { reset(); }


// The single definition of "ready". Change only this function.
bool ReadyTaskIterator::isReadyItem(WorkComponent* node) 
{
    const WorkItemState* state = node->getState();
    if (state == nullptr) 
    {
        return false;   //Stages have no state
    }
    //TODO: replace with the real check once WorkItemState's API is known,
    //e.g. return state->isAvailable();
    //Placeholder: treats every leaf that has a state as ready.
    return true;
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


// Depth-first walk that stops at the next ready leaf.
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
                ++n;
            }
            for (int i = n - 1; i >= 0; --i) 
            {
                pending.push(node->getChild(i));
            }
        } else if (isReadyItem(node)) {
            forward = node;
            return;
        }
    }
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