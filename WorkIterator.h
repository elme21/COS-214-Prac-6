#ifndef WORKITERATOR_H
#define WORKITERATOR_H

#include "WorkComponent.h"

class WorkIterator
{
    public:
        virtual bool hasNext() = 0;
        virtual WorkComponent* next() = 0;
        virtual void reset() = 0;
        virtual WorkComponent* currentItem() = 0;
};

#endif