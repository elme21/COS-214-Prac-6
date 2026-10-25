
#include "CodeDefinitionBuilder.h"

#include <stdexcept>
#include <memory>
#include <string>

CodeDefinitionBuilder::CodeDefinitionBuilder(const std::string& name)
{
    
}

void CodeDefinitionBuilder::addStage(const std::string& name, const std::string& parent)
{

}

void CodeDefinitionBuilder::addTask(
    const std::string& name,
    const std::string& kind,
    const std::string& stage)
{
}

void CodeDefinitionBuilder::setAssignmentStrategy(const std::string& task,AssignmentStrategy* strategy)
{

}

void CodeDefinitionBuilder::addDecorator(const std::string& task,const std::string& decorator)
{
}

void CodeDefinitionBuilder::addDependency(const std::string& from,const std::string& to)
{
}

void CodeDefinitionBuilder::setApprovalChain(const std::string& task, ApprovalHandler* handler)
{
}

std::shared_ptr<WorkflowDefinition>CodeDefinitionBuilder::getDefinition()
{
    return definition;
}

//Please note that this still needs a lot of work.