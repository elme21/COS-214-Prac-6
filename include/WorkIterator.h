#ifndef WORKITERATOR_H
#define WORKITERATOR_H

class WorkComponent;

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