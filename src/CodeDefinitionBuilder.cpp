#include "../include/CodeDefinitionBuilder.h"
#include "../include/WorkStage.h"
#include "../include/WorkItem.h"
#include "../include/ManualTask.h"
#include "../include/AutomatedTask.h"
#include "../include/ApprovalTask.h"
#include "../include/AuditDecorator.h"

#include <stdexcept>
#include <memory>
#include <iostream>
#include <algorithm>

CodeDefinitionBuilder::CodeDefinitionBuilder(const std::string& n):workflowName(n)
{}

void CodeDefinitionBuilder::addStage(const std::string& n,const std::string& p){

 if(stages.count(n)||tasks.count(n))
    throw std::invalid_argument("Duplicate name: "+n);

 if(p.empty()){
    if(definition)
        throw std::logic_error("Root already defined");

    std::shared_ptr<WorkComponent> root(new WorkStage());
    stages[n]=static_cast<WorkStage*>(root.get());
    definition=std::make_shared<WorkflowDefinition>(workflowName,root);
 }else{
    auto it=stages.find(p);

    if(it==stages.end())
        throw std::invalid_argument("Unknown parent stage: "+p);

     WorkStage* stage=new WorkStage();
     it->second->add(stage);
     stages[n]=stage;
 }
}

void CodeDefinitionBuilder::addTask(const std::string& n,const std::string& kind,const std::string& stage){

 if(stages.count(n)||tasks.count(n))
    throw std::invalid_argument("Duplicate name: "+n);

 auto it=stages.find(stage);

 if(it==stages.end())
    throw std::invalid_argument("Unknown stage: "+stage);

 WorkItem* item=nullptr;

 if(kind=="manual")
    item=new ManualTask(n,n);
    else if(kind=="automated")
    item=new AutomatedTask(n,n);
        else if(kind=="approval")
        item=new ApprovalTask(n,n);
            else
             throw std::invalid_argument("Unknown task kind: "+kind);

    it->second->add(item);
    tasks[n]=item;
    taskComponents[n]=item;
    parents[n]=it->second;
}

void CodeDefinitionBuilder::setAssignmentStrategy(const std::string& n,AssignmentStrategy* s){

 if(!tasks.count(n)||!s)
    throw std::invalid_argument("Invalid task/strategy: "+n);

 definition->setStrategy(n,s);
}

void CodeDefinitionBuilder::addDecorator(const std::string& n,const std::string& kind){

 auto it=taskComponents.find(n);

 if(it==taskComponents.end())
    throw std::invalid_argument("Unknown task: "+n);

 WorkComponent* old=it->second;

 // Construct first: no ownership is transferred until a constructor succeeds.
 WorkComponent* replacement=nullptr;

 if(kind=="audit")
    replacement=new AuditDecorator(old,n,[](const std::string& msg){std::cout<<msg<<'\n';});
 else
    throw std::invalid_argument("Unknown decorator: "+kind);

 // WorkStage::remove does not delete the child; decorator now owns old.
 parents.at(n)->remove(old);
 parents.at(n)->add(replacement);
 it->second=replacement;
}

void CodeDefinitionBuilder::addDependency(const std::string& from,const std::string& to){

 if(!tasks.count(from)||!tasks.count(to)||from==to)
    throw std::invalid_argument("Invalid dependency: "+from+" -> "+to);

 definition->addDependency(from,to);
}

void CodeDefinitionBuilder::setApprovalChain(const std::string& n,ApprovalHandler* handler){

 auto it=tasks.find(n);

 if(it==tasks.end())
    throw std::invalid_argument("Unknown task: "+n);

 ApprovalTask* approval=dynamic_cast<ApprovalTask*>(it->second);

 if(!approval)
    throw std::invalid_argument("Not an approval task: "+n);

 approval->setFirstHandler(handler);
}

std::shared_ptr<WorkflowDefinition> CodeDefinitionBuilder::getDefinition(){

 if(!definition)
    throw std::logic_error("No root stage");

 return definition;
}
