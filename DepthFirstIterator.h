#ifndef DEPTHFIRSTITERATOR_H
#define DEPTHFIRSTITERATOR_H

#include "WorkIterator.h"

class DepthFirstIterator : public WorkIterator
{
    public:
        DepthFirstIterator(WorkComponent* c);
        bool hasNext() override;
        WorkComponent* next() override;
        void reset() override;
        WorkComponent* currentItem() override;

        /*
        WorkComponent* first() override;
        WorkComponent* next() override;
        bool isDone() override;
        WorkComponent* currentItem() override;
        */
};

#endif