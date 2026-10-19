#ifndef WORKCOMPONENT_H
#define WORKCOMPONENT_H

class WorkComponent
{
    public:
        virtual ~WorkComponent() {};

        //Every Component must implement:
        virtual void execute() = 0;
        virtual WorkComponent* clone() = 0;
        //virtual WorkIterator* createIterator() = 0;

        //WorkStage overrides:
        virtual void add(WorkComponent* c) = 0;
        virtual void remove(WorkComponent* c) = 0;    
        virtual WorkComponent* getChild(int i) = 0;
};

#endif