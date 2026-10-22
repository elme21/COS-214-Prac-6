#include "DepthFirstIterator.h"
#include "WorkComponent.h"

DepthFirstIterator::DepthFirstIterator(WorkComponent* r): root(r), current(nullptr) { reset(); }


void DepthFirstIterator::reset() 
{
    while (!pending.empty()) 
    {
        pending.pop();
    }

    current = nullptr;
    if (root != nullptr) 
    {
        pending.push(root);
    }
}


bool DepthFirstIterator::hasNext() 
{
    return !pending.empty();
}

//Pre-order visiting: visit a node, then its children left to right.
WorkComponent* DepthFirstIterator::next() 
{
    if (pending.empty()) 
    {
        return nullptr;
    }
    WorkComponent* node = pending.top();
    pending.pop();

    //Count children: getChild returns nullptr once the index is out of range
    int n = 0;
    while (node->getChild(n) != nullptr) 
    {
        ++n;
    }

    //Push in reverse so the first child is popped first
    for (int i = n - 1; i >= 0; --i) 
    {
        pending.push(node->getChild(i));
    }

    current = node;
    return node;
}


WorkComponent* DepthFirstIterator::currentItem() 
{
    return current;
}