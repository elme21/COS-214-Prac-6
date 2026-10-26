#ifndef DEPTHFIRSTITERATOR_H
#define DEPTHFIRSTITERATOR_H

#include <stack>
#include "WorkIterator.h"

class DepthFirstIterator : public WorkIterator 
{
    private:
        WorkComponent* root;                  //Not owned
        std::stack<WorkComponent*> pending;   //Nodes still to visit
        WorkComponent* current;

    public:
        explicit DepthFirstIterator(WorkComponent* root);

        bool hasNext() override;
        WorkComponent* next() override;
        void reset() override;
        WorkComponent* currentItem() override;
};

#endif