#ifndef DEPTHFIRSTITERATOR_H
#define DEPTHFIRSTITERATOR_H

#include "WorkIterator.h"

class DepthFirstIterator : public WorkIterator
{
    public:
        bool hasNext();
        WorkComponent* next();
        void reset();
        WorkComponent* currentItem();
};

#endif