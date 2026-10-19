#include "WorkStage.h"

WorkStage::~WorkStage() 
{
    for (int i = 0; i < children.size(); i++) 
    {
        delete children[i];
    }
}


void WorkStage::add(WorkComponent* c)
{
    children.push_back(c);
}


void WorkStage::remove(WorkComponent* c) 
{
    for (int i = 0; i < children.size(); i++) 
    {
        if (children[i] == c) 
        {
            children.erase(children.begin() + i);
            return;
        }
    }
}


WorkComponent* WorkStage::getChild(int i)
{
    return children[i];
}


void WorkStage::execute() 
{
    for (int i = 0; i < children.size(); i++) 
    {
        children[i]->execute();
    }
}


WorkComponent *WorkStage::clone()
{
    //Part of Prototype
}