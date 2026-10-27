#ifndef READYTASKITERATOR_H
#define READYTASKITERATOR_H

#include <stack>
#include "WorkIterator.h"

class ReadyTaskIterator : public WorkIterator 
{
    private:
        WorkComponent* root;                    //Not owned
        std::stack<WorkComponent*> pending;     //Nodes still to visit
        WorkComponent* forward;                 //Next ready item
        WorkComponent* current;

        void advance();                                 //Finds the next ready item
        static bool isReadyItem(WorkComponent* node);   //Defines what it means to be "ready"

    public:
        explicit ReadyTaskIterator(WorkComponent* root);

        bool hasNext() override;
        WorkComponent* next() override;
        void reset() override;
        WorkComponent* currentItem() override;
};

#endif