#include "../include/WorkflowManager.h"
#include "../include/DefinitionRegistry.h"
#include "../include/WorkflowDefinition.h"
#include "../include/WorkflowInstance.h"
#include "../include/WorkComponent.h"
#include "../include/WorkComponentDecorator.h"
#include "../include/WorkStage.h"
#include "../include/WorkItem.h"
#include "../include/WorkItemState.h"
#include "../include/ResourceCoordinator.h"
#include "../include/TaskRouter.h"
#include "../include/CommandProcessor.h"
#include "../include/CompleteCommand.h"
#include "../include/AssignCommand.h"
#include "../include/StartCommand.h"
#include "../include/ApproveCommand.h"
#include "../include/RejectCommand.h"
#include "../include/CancelCommand.h"
#include "../include/EscalateCommand.h"
#include "../include/MakeAvailableCommand.h"
#include "../include/InstanceSnapshot.h"

#include <stdexcept>
#include <functional>
#include <vector>
#include <memory>

WorkflowManager::WorkflowManager(DefinitionRegistry* r,TaskRouter* t):registry(r),router(t)
{
 if(!r||!t)
    throw std::invalid_argument("WorkflowManager requires registry and router");
}

std::string WorkflowManager::createInstance(const std::string& name){

 auto def=registry->get(name);

 if(!def)
    throw std::invalid_argument("Unknown definition: "+name);

 auto root=def->cloneRoot();

 Runtime run;
 run.instance=std::make_shared<WorkflowInstance>(root,"Created");
 run.definition=def;
 run.mediator=std::make_shared<ResourceCoordinator>();
 run.commands=std::make_shared<CommandProcessor>();

index(run);

 std::string id="instance-"+std::to_string(nextId++);
 instances.emplace(id,std::move(run));

 return id;
}

void WorkflowManager::index(Runtime& run){

 std::function<void(WorkComponent*)> walk=[&](WorkComponent* node){

    if(auto* decorator=dynamic_cast<WorkComponentDecorator*>(node)){

        WorkComponent* wrapped=decorator->getWrappedComponent();

        walk(wrapped);

        while(auto* innerDecorator=dynamic_cast<WorkComponentDecorator*>(wrapped))
            wrapped=innerDecorator->getWrappedComponent();

        if(auto* item=dynamic_cast<WorkItem*>(wrapped)){
            const std::string id=item->getId().empty()?item->getName():item->getId();
            run.components[id]=node;
        }

        return;
    }

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

 walk(run.instance->getRoot().get());

 for(const auto& edge:run.definition->getDependencies()){

    auto a=run.items.find(edge.first);
    auto b=run.items.find(edge.second);

    if(a==run.items.end()||b==run.items.end())
        throw std::logic_error("Dependency references task not accessible after cloning: "+edge.first+" -> "+edge.second);

    run.mediator->addDependency(b->second,a->second);
 }
}

void WorkflowManager::start(const std::string& id){

 auto it=instances.find(id);

 if(it==instances.end())
    throw std::invalid_argument("Unknown instance: "+id);

 Runtime& run=it->second;

 if(run.instance->getStatus()!="Created")
    throw std::logic_error("Instance already started");

 for(auto& pair:run.items){

    WorkItem* item=pair.second;

    if(run.mediator->canBecomeAvailable(item)){

        item->makeAvailable();

        if(auto* s=run.definition->getStrategy(pair.first)){
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

 auto it=instances.find(id);

 if(it==instances.end())
    throw std::invalid_argument("Unknown instance: "+id);

 Runtime& run=it->second;

 if(run.instance->getStatus()!="Running")
    throw std::logic_error("Instance is not running");

 auto found=run.items.find(taskId);

 if(found==run.items.end())
    throw std::invalid_argument("Unknown task ID: "+taskId);

 WorkItem* item=found->second;

 if(!run.mediator->canBecomeAvailable(item))
    throw std::logic_error("Task prerequisites not met: "+taskId);

 // State transitions must be respected: only an InProgress item may complete.
 if(!item->getState()||item->getState()->name()!="InProgress")
    throw std::logic_error("Task must be InProgress before completion: "+taskId);

 run.commands->run(std::unique_ptr<WorkflowCommand>(new CompleteCommand(run.components.at(taskId))));

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
WorkflowInstance* WorkflowManager::getInstance(const std::string& id){
 auto it=instances.find(id);
 if(it==instances.end()) throw std::invalid_argument("Unknown instance: "+id);
 return it->second.instance.get();
}

WorkComponent* WorkflowManager::getTree(const std::string& id){
 return getInstance(id)->getRoot().get();
}

WorkComponent* WorkflowManager::getTask(const std::string& id,const std::string& taskId){
 auto it=instances.find(id);
 if(it==instances.end()) throw std::invalid_argument("Unknown instance: "+id);
 auto task=it->second.components.find(taskId);
 if(task==it->second.components.end()) throw std::invalid_argument("Unknown task ID: "+taskId);
 return task->second;
}

ResourceCoordinator* WorkflowManager::getMediator(const std::string& id){
 auto it=instances.find(id);
 if(it==instances.end()) throw std::invalid_argument("Unknown instance: "+id);
 return it->second.mediator.get();
}

CommandProcessor* WorkflowManager::getCommands(const std::string& id){
 auto it=instances.find(id);
 if(it==instances.end()) throw std::invalid_argument("Unknown instance: "+id);
 return it->second.commands.get();
}

bool WorkflowManager::runTaskCommand(const std::string& id,const std::string& taskId,
                                     const std::string& action,Participant* participant){
 auto it=instances.find(id);
 if(it==instances.end()) throw std::invalid_argument("Unknown instance: "+id);
 Runtime& run=it->second;
 if(run.instance->getStatus()!="Running") throw std::logic_error("Instance is not running");
 WorkComponent* component=getTask(id,taskId);
 WorkItem* item=run.items.at(taskId);
 if((action=="makeAvailable"||action=="assign"||action=="start") &&
    !run.mediator->canBecomeAvailable(item))
    throw std::logic_error("Task prerequisites not met: "+taskId);
 std::unique_ptr<WorkflowCommand> command;
 if(action=="makeAvailable") command.reset(new MakeAvailableCommand(component));
 else if(action=="assign") command.reset(new AssignCommand(component,participant));
 else if(action=="start") command.reset(new StartCommand(component));
 else if(action=="approve") command.reset(new ApproveCommand(component));
 else if(action=="reject") command.reset(new RejectCommand(component));
 else if(action=="cancel") command.reset(new CancelCommand(component));
 else if(action=="escalate") command.reset(new EscalateCommand(component));
 else throw std::invalid_argument("Unknown action: "+action);
 return run.commands->run(std::move(command));
}

bool WorkflowManager::makeAvailable(const std::string& id,const std::string& taskId){
 return runTaskCommand(id,taskId,"makeAvailable");
}

bool WorkflowManager::assign(const std::string& id,const std::string& taskId,Participant* participant){
 if(!participant) throw std::invalid_argument("Cannot assign a null participant");
 return runTaskCommand(id,taskId,"assign",participant);
}

bool WorkflowManager::start(const std::string& id,const std::string& taskId){
 return runTaskCommand(id,taskId,"start");
}

bool WorkflowManager::approve(const std::string& id,const std::string& taskId){
 return runTaskCommand(id,taskId,"approve");
}

bool WorkflowManager::reject(const std::string& id,const std::string& taskId){
 return runTaskCommand(id,taskId,"reject");
}

bool WorkflowManager::cancel(const std::string& id,const std::string& taskId){
 return runTaskCommand(id,taskId,"cancel");
}

bool WorkflowManager::escalate(const std::string& id,const std::string& taskId){
 return runTaskCommand(id,taskId,"escalate");
}

void WorkflowManager::undo(const std::string& id){
 getCommands(id)->undo();
}

WorkItem* WorkflowManager::getItem(const std::string& id,
                                   const std::string& taskId){
 auto it=instances.find(id);
 if(it==instances.end()) return nullptr;

 auto item=it->second.items.find(taskId);
 return item==it->second.items.end()?nullptr:item->second;
}

std::vector<std::string> WorkflowManager::getTaskIds(const std::string& id){
 std::vector<std::string> ids;

 auto it=instances.find(id);
 if(it==instances.end()) return ids;

 for(const auto& pair:it->second.items)
    ids.push_back(pair.first);

 return ids;
}

void WorkflowManager::restoreSnapshot(const std::string& id,
                                      std::shared_ptr<InstanceSnapshot> snapshot){

 auto it=instances.find(id);

 if(it==instances.end()||!snapshot)
    return;

 Runtime& run=it->second;

 //clear command history before replacing the tree
 run.commands=std::make_shared<CommandProcessor>();

 //restore the saved workflow state and cloned component tree
 run.instance->restoreSnapshot(snapshot);

 //old task pointers and mediator registrations are no longer valid
 run.components.clear();
 run.items.clear();
 run.mediator=std::make_shared<ResourceCoordinator>();

 //rebuild indexes and dependencies for the restored tree
 index(run);
}