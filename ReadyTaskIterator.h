#ifndef READYTASKITERATOR_H
#define READYTASKITERATOR_H

#include "WorkIterator.h"

class ReadyTasktIterator : public WorkIterator
{
    public:
        bool hasNext();
        WorkComponent* next();
        void reset();
        WorkComponent* currentItem();
};

#endif 