#include "ScenarioRuntime.h"

#include "GenericDirector.h"
#include "TaskLookup.h"

#include "CodeDefinitionBuilder.h"
#include "DefinitionRegistry.h"
#include "WorkflowDefinition.h"
#include "WorkItem.h"
#include "Participant.h"
#include "Resource.h"
#include "ApprovalHandler.h"
#include "RoleApproverHandler.h"
#include "FinalApproverHandler.h"
#include "AssignmentStrategy.h"
#include "RoleBasedAssignment.h"
#include "WorkloadAssignment.h"

#include <cctype>

namespace {
std::string slugOf(const std::string& name) {
    std::string out;
    for (size_t i = 0; i < name.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(name[i]);
        if (std::isalnum(c)) out += static_cast<char>(std::tolower(c));
        else if (!out.empty() && out[out.size() - 1] != '-') out += '-';
    }
    while (!out.empty() && out[out.size() - 1] == '-') out.erase(out.size() - 1);
    return out.empty() ? "scenario" : out;
}
}

ScenarioRuntime::ScenarioRuntime(const ScenarioSpec& spec)
    : scenario(spec), slug(slugOf(spec.name)), built(nullptr), counter(0) {
    std::vector<const TaskSpec*> tasks = scenario.allTasks();

    for (size_t i = 0; i < scenario.participants.size(); ++i) {
        const ParticipantSpec& p = scenario.participants[i];
        people.push_back(std::unique_ptr<Participant>(new Participant(p.id, p.name, p.role)));
    }
    for (size_t i = 0; i < scenario.resources.size(); ++i) {
        const ResourceSpec& r = scenario.resources[i];
        resources.push_back(std::unique_ptr<Resource>(new Resource(r.name, r.capacity)));
    }

    std::vector<Participant*> everyone;
    for (size_t i = 0; i < people.size(); ++i) everyone.push_back(people[i].get());

    for (size_t i = 0; i < tasks.size(); ++i) {
        const TaskSpec& t = *tasks[i];

        // Approval chain: level 1 -> level 2 -> ... -> final approver (ApprovalTask keeps the result).
        if (t.type == "approval" && !t.chain.empty()) {
            ApprovalHandler* next = nullptr;
            std::vector<ChainLevelInfo> info;
            for (size_t k = t.chain.size(); k > 0; --k) {
                const ChainLevelSpec& level = t.chain[k - 1];
                bool isFinal = k == t.chain.size();
                std::string label = level.label;
                if (label.empty()) label = isFinal ? std::string("Final approver") : "Approver level " + std::to_string(k);
                ApprovalHandler* real = nullptr;
                if (isFinal) {
                    real = new FinalApproverHandler(label, level.limit);
                } else {
                    real = new RoleApproverHandler(label, level.role, level.limit, next);
                }
                handlers.push_back(std::unique_ptr<ApprovalHandler>(real));
                next = real;
                ChainLevelInfo li = { label, level.role, level.limit, isFinal };
                info.insert(info.begin(), li);
            }
            chainHeads[t.id] = next;
            chainInfo[t.id] = info;
        }

        // Assignment strategy (Strategy pattern): candidates are the scenario's participants.
        if (t.strategy == "role") {
            AssignmentStrategy* s = new RoleBasedAssignment(everyone, t.role);
            strategyObjects.push_back(std::unique_ptr<AssignmentStrategy>(s));
            strategies[t.id] = s;
        } else if (t.strategy == "workload") {
            std::vector<Participant*> withRole;
            for (size_t p = 0; p < people.size(); ++p) if (people[p]->getRole() == t.role) withRole.push_back(people[p].get());
            AssignmentStrategy* s = new WorkloadAssignment(withRole);
            strategyObjects.push_back(std::unique_ptr<AssignmentStrategy>(s));
            strategies[t.id] = s;
        }
    }
}

ScenarioRuntime::~ScenarioRuntime() {}

bool ScenarioRuntime::build(DefinitionRegistry& registry, std::string& error, std::vector<std::string>& steps) {
    if (registry.get(scenario.name) != nullptr) {
        error = "A scenario named '" + scenario.name + "' already exists. Choose another name.";
        return false;
    }
    builder.reset(new CodeDefinitionBuilder(scenario.name));
    GenericDirector director(scenario, chainHeads, strategies);
    director.construct(*builder);
    std::shared_ptr<WorkflowDefinition> definition = builder->getDefinition();

    // Display names (the builder uses the id as name; WorkItem::setName is the real setter).
    std::vector<TaskRef> tasks = allTasks(definition->getRoot().get());
    for (size_t i = 0; i < tasks.size(); ++i) {
        const TaskSpec* t = scenario.task(tasks[i].item->getId());
        if (t != nullptr) tasks[i].item->setName(t->name);
    }

    // The registry owns a raw pointer while the builder hands out a shared_ptr, so it gets its own
    // copy (which shares the same prototype root).
    if (!registry.registerDefinition(new WorkflowDefinition(*definition))) {
        error = "DefinitionRegistry refused '" + scenario.name + "'.";
        return false;
    }
    built = registry.get(scenario.name);
    names.learn(scenario, built->getRoot().get());

    steps.push_back("GenericDirector::construct(CodeDefinitionBuilder)");
    steps.push_back("DefinitionRegistry::registerDefinition(\"" + scenario.name + "\")");
    return true;
}

Participant* ScenarioRuntime::participant(const std::string& id) const {
    for (size_t i = 0; i < people.size(); ++i) if (people[i]->getID() == id) return people[i].get();
    return nullptr;
}

Resource* ScenarioRuntime::resource(const std::string& id) const {
    for (size_t i = 0; i < scenario.resources.size(); ++i) {
        if (scenario.resources[i].id == id) return resources[i].get();
    }
    return nullptr;
}

std::string ScenarioRuntime::resourceIdOf(const Resource* r) const {
    for (size_t i = 0; i < resources.size(); ++i) if (resources[i].get() == r) return scenario.resources[i].id;
    return "";
}

Resource* ScenarioRuntime::resourceFor(const std::string& taskId, const std::string& participantId) const {
    const TaskSpec* t = scenario.task(taskId);
    if (t == nullptr || t->resource.empty()) return nullptr;
    if (t->resource != "@assignee") return resource(t->resource);
    for (size_t i = 0; i < scenario.resources.size(); ++i) {
        if (scenario.resources[i].owner == participantId) return resources[i].get();
    }
    return nullptr;
}

AssignmentStrategy* ScenarioRuntime::strategyFor(const std::string& taskId) const {
    std::map<std::string, AssignmentStrategy*>::const_iterator it = strategies.find(taskId);
    return it == strategies.end() ? nullptr : it->second;
}

std::string ScenarioRuntime::strategyName(const std::string& taskId) const {
    const TaskSpec* t = scenario.task(taskId);
    if (t == nullptr || strategyFor(taskId) == nullptr) return "";
    return t->strategy == "role" ? "RoleBasedAssignment" : "WorkloadAssignment";
}

std::vector<ChainLevelInfo> ScenarioRuntime::chainOf(const std::string& taskId) const {
    std::map<std::string, std::vector<ChainLevelInfo> >::const_iterator it = chainInfo.find(taskId);
    return it == chainInfo.end() ? std::vector<ChainLevelInfo>() : it->second;
}
