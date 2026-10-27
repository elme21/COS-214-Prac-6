#ifndef WORKITERATOR_H
#define WORKITERATOR_H

class WorkComponent;

/**
 * Abstract parent class of Iterator pattern, only interacts
 * with Composite pattern, has no base implementation
 */
class WorkIterator
{
    public:
        virtual ~WorkIterator() {}

        virtual bool hasNext() = 0;
        virtual WorkComponent* next() = 0;      
        virtual void reset() = 0;
        virtual WorkComponent* currentItem() = 0;
};

#endif