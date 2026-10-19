#ifndef WORKSTAGE_H
#define WORKSTAGE_H

#include "WorkComponent.h"

#include <vector>

class WorkStage : public WorkComponent
{
    private:
        std::vector<WorkComponent*> children;
    public:
        WorkStage() {};
        virtual ~WorkStage();

        void add(WorkComponent* c) override;
        void remove(WorkComponent* c) override;
        WorkComponent* getChild(int i) override;

        void execute() override;
        WorkComponent* clone() override;
        //WorkIterator* createIterator() override;
};

#endif