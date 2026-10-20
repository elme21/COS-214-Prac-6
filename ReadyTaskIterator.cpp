#include "ReadyTaskIterator.h"

bool ReadyTaskIterator::hasNext()
{
    if (this->next() != nullptr)
    {
        return true;
    }
    return false;
}


WorkComponent* ReadyTaskIterator::next()
{

}


void ReadyTaskIterator::reset()
{

}


WorkComponent* ReadyTaskIterator::currentItem()
{
    
}