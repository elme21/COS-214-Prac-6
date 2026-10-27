#include "../include/WorkflowManager.h"
#include "../include/DefinitionRegistry.h"
#include "../include/WorkflowDefinition.h"
#include "../include/WorkflowInstance.h"
#include "../include/WorkComponent.h"
#include "../include/WorkStage.h"
#include "../include/WorkItem.h"
#include "../include/WorkItemState.h"
#include "../include/ResourceCoordinator.h"
#include "../include/TaskRouter.h"
#include "../include/CommandProcessor.h"
#include "../include/CompleteCommand.h"
#include <stdexcept>
#include <functional>
#include <vector>
#include <memory>

WorkflowManager::WorkflowManager(DefinitionRegistry* r,TaskRouter* t):registry(r),router(t)
{
 if(!r || !t)
    throw std::invalid_argument("WorkflowManager requires registry and router");
}

std::string WorkflowManager::createInstance(const std::string& name)
{
 //find & clone workflow
 auto def=registry->get(name);

 if(!def)
    throw std::invalid_argument("Unknown definition: "+name);

 auto root=def->cloneRoot();

 //setup runtime holding workflow instance and subsystems it needs
 Runtime run;
 run.instance=std::make_shared<WorkflowInstance>(root,"Created");
 run.definition=def;
 run.mediator=std::make_shared<ResourceCoordinator>();
 run.commands=std::make_shared<CommandProcessor>();

    //find every task inside workflow (composite tree)
    std::function<void(WorkComponent*)> walk=[&](WorkComponent* node){

        if(auto* item=dynamic_cast<WorkItem*>(node)){

         const std::string id=item->getId().empty()?item->getName():item->getId();

         if(id.empty()||run.components.count(id))
            throw std::logic_error("Duplicate/empty task ID: "+id);

          run.components[id]=node;
         run.items[id]=item;
         run.mediator->registerColleague(item);
         return;
        }

        for(int i=0;;++i){
            WorkComponent* child=node->getChild(i);

         if(!child)
            break;

         walk(child);
        }
    };

 walk(root.get());

 //register dependaancies with mediator
 for(const auto& edge:def->getDependencies()){

    auto a=run.items.find(edge.first);
    auto b=run.items.find(edge.second);

    if(a==run.items.end()||b==run.items.end())
        throw std::logic_error("Dependency references task not accessible after cloning: "+edge.first+" -> "+edge.second);

        run.mediator->addDependency(b->second,a->second); 
    }

 //Save the instance
 std::string id="instance-"+std::to_string(nextId++);
 instances.emplace(id,std::move(run));

 return id;
}

void WorkflowManager::start(const std::string& id){ //start if prerequisites met and delegates to strategy

 auto it=instances.find(id);

 if(it==instances.end())
    throw std::invalid_argument("Unknown instance: "+id);

 Runtime& run=it->second;

 if(run.instance->getStatus()!="Created")
    throw std::logic_error("Instance already started");

  for(auto& pair:run.items){

    WorkItem* item=pair.second;

    if(run.mediator->canBecomeAvailable(item)){  //prerequisites met

        item->makeAvailable();

        if(auto* s=run.definition->getStrategy(pair.first)){ //assignment strategy exists?
            router->setStrategy(s);
            router->route(item);
        }
    }
  }

 run.instance->setStatus("Running");
}

void WorkflowManager::complete(const std::string& compositeId){

 auto pos=compositeId.find(':');

 if(pos==std::string::npos)
    throw std::invalid_argument("Use instanceId:taskId, or complete(instanceId,taskId)");

 completeInInstance(compositeId.substr(0,pos),compositeId.substr(pos+1));
}

void WorkflowManager::complete(const std::string& instanceId,const std::string& taskId){

 completeInInstance(instanceId,taskId);
}

void WorkflowManager::completeInInstance(const std::string& id,const std::string& taskId){
//validate request
 auto it=instances.find(id);

 if(it==instances.end())
    throw std::invalid_argument("Unknown instance: "+id);

 Runtime& run=it->second;

 if(run.instance->getStatus()!="Running")
    throw std::logic_error("Instance is not running");

//find task
 auto found=run.items.find(taskId);

 if(found==run.items.end())
    throw std::invalid_argument("Unknown task ID: "+taskId);

 WorkItem* item=found->second;

 //checks with mediator and state 
 if(!run.mediator->canBecomeAvailable(item))
    throw std::logic_error("Task prerequisites not met: "+taskId);

 if(!item->getState()||item->getState()->name()!="InProgress")
    throw std::logic_error("Task must be InProgress before completion: "+taskId);

//use Command to encapsulate and complete task.
 run.commands->run(std::unique_ptr<WorkflowCommand>(new CompleteCommand(item)));

 //notify mediator and check for next tasks to make available
 if(!item->isComplete())
    throw std::logic_error("Completion was refused: "+taskId);

 item->changed("completed");

 bool allDone=true;

 for(auto& pair:run.items){

    WorkItem* next=pair.second;

    if(!next->isComplete())
        allDone=false;

    if(next->getState()&&next->getState()->name()=="Created"&&run.mediator->canBecomeAvailable(next)){

        next->makeAvailable();

        if(auto* strategy=run.definition->getStrategy(pair.first)){
            router->setStrategy(strategy);
            router->route(next);
        }
    }
 }

 if(allDone)
    run.instance->setStatus("Completed");
}

std::string WorkflowManager::getStatus(const std::string& id){

 auto it=instances.find(id);

 if(it==instances.end())
    throw std::invalid_argument("Unknown instance: "+id);

 return it->second.instance->getStatus();
}
