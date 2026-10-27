#include "DefinitionRegistry.h"
#include "WorkflowDefinition.h"
#include "WorkComponent.h"
#include <algorithm>
#include <stdexcept>

void DefinitionRegistry::registerDefinition(std::shared_ptr<WorkflowDefinition> d){

 if(!d)
    throw std::invalid_argument("Null definition");

 for(auto& existing:definitions){
    if(existing->getName()==d->getName()){
        existing=d;
        return;
    }
 }

 definitions.push_back(std::move(d));
}

std::shared_ptr<WorkflowDefinition> DefinitionRegistry::get(const std::string& name)const{

 for(const auto& d:definitions){
    if(d->getName()==name)
        return d;
 }

 return nullptr;
}

void DefinitionRegistry::addProto(WorkComponent* p){

 if(p)
    prototypeStore.push_back(p);
}

void DefinitionRegistry::removeProto(WorkComponent* p){

 prototypeStore.erase(std::remove(prototypeStore.begin(),prototypeStore.end(),p),prototypeStore.end());
}

WorkComponent* DefinitionRegistry::getProto()const{

 return prototypeStore.empty()?nullptr:prototypeStore.back();
}
