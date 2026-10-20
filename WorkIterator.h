#ifndef WORKITERATOR_H
#define WORKITERATOR_H

class WorkComponent;

class WorkIterator
{
    public:
        virtual bool hasNext() = 0;
        virtual WorkComponent* next() = 0;
        virtual void reset() = 0;
        virtual WorkComponent* currentItem() = 0;

        /*
        WorkComponent* first() = 0;
        WorkComponent* next() = 0;
        bool isDone() = 0;
        WorkComponent* currentItem() = 0;
        */
};

#endif