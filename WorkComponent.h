#ifndef WORKCOMPONENT_H
#define WORKCOMPONENT_H

class WorkIterator;

class WorkComponent
{
    public:
        virtual ~WorkComponent() {};

        virtual void add(WorkComponent* c) = 0;
        virtual void remove(WorkComponent* c) = 0;    
        virtual WorkComponent* getChild(int i) = 0;

        virtual void execute() = 0;
        virtual WorkComponent* clone() = 0;
        virtual WorkIterator* createIterator() = 0;
};

#endif