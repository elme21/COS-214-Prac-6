#ifndef READYTASKITERATOR_H
#define READYTASKITERATOR_H

#include "WorkIterator.h"

class ReadyTaskIterator : public WorkIterator
{
    public:
        ReadyTaskIterator();
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