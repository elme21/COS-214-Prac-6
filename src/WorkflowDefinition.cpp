#include "../include/WorkflowDefinition.h"
#include "../include/WorkComponent.h"
#include <stdexcept>
#include <utility>

WorkflowDefinition::WorkflowDefinition(const std::string& n,std::shared_ptr<WorkComponent> r):name(n),root(std::move(r))
{}

const std::string& WorkflowDefinition::getName()const{

 return name;
}

std::shared_ptr<WorkComponent> WorkflowDefinition::getRoot()const{

 return root;
}

std::shared_ptr<WorkComponent> WorkflowDefinition::cloneRoot()const{

 if(!root)
    throw std::logic_error("Definition has no root");

 return std::shared_ptr<WorkComponent>(root->clone());
}

void WorkflowDefinition::addDependency(const std::string& a,const std::string& b){

 dependencies.emplace_back(a,b);
}

const std::vector<std::pair<std::string,std::string>>& WorkflowDefinition::getDependencies()const{

 return dependencies;
}
void WorkflowDefinition::setStrategy(const std::string& n,AssignmentStrategy* s){

 strategies[n]=s;
}

AssignmentStrategy* WorkflowDefinition::getStrategy(const std::string& n)const{

 auto it=strategies.find(n);

 return it==strategies.end()?nullptr:it->second;
}

void WorkflowDefinition::keepApprovalHandler(ApprovalHandler*){

}
