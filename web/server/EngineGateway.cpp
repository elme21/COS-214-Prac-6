#include "EngineGateway.h"

#include "Trace.h"
#include "HistoryLog.h"
#include "WebCommands.h"
#include "PendingIntegration.h"

// Industry-specific code (web/scenarios/)
#include "Scenario.h"
#include "EducationScenario.h"
#include "HealthcareScenario.h"
#include "RecordingDefinitionBuilder.h"
#include "ParticipantDirectory.h"
#include "NameLookup.h"
#include "TestConnectors.h"
#include "ChainTrace.h"
#include "RuleHelpers.h"

// The real TIVIDY engine (include/)
#include "WorkComponent.h"
#include "WorkItem.h"
#include "WorkItemState.h"
#include "AutomatedTask.h"
#include "ApprovalTask.h"
#include "WorkflowDefinition.h"
#include "WorkflowInstance.h"
#include "InstanceSnapshot.h"
#include "SnapshotHistory.h"
#include "CommandProcessor.h"
#include "ReadyTaskIterator.h"
#include "ResourceCoordinator.h"
#include "Resource.h"
#include "Participant.h"
#include "ConnectorManager.h"   // web/temporary_stubs until Member 7's real class exists

#include <chrono>
#include <ctime>
#include <iostream>
#include <set>
#include <sstream>

typedef std::map<std::string, std::string> Data;
using nlohmann::json;

// ===========================================================================
// Internal records
// ===========================================================================

/** Everything one industry needs, rebuilt from scratch by reset(). */
struct EngineGateway::Industry {
    std::unique_ptr<Scenario> scenario;
    std::unique_ptr<ResourceCoordinator> coordinator;   // Mediator shared by the industry's instances
    std::unique_ptr<ParticipantDirectory> people;
    std::unique_ptr<ConnectorManager> connectors;
    std::map<std::string, std::unique_ptr<TestConnector> > testConnectors;
    std::unique_ptr<RecordingDefinitionBuilder> builder;
    std::shared_ptr<WorkflowDefinition> definition;     // stands in for DefinitionRegistry
    std::vector<TaskInfo> tasks;
    std::vector<TraceStep> buildTrace;
    int counter;

    Industry() : counter(0) {}

    const TaskInfo* info(const std::string& id) const {
        for (size_t i = 0; i < tasks.size(); ++i) if (tasks[i].id == id) return &tasks[i];
        return nullptr;
    }
    std::string kind(const std::string& id) const {
        const std::vector<RecordingDefinitionBuilder::TaskRecord>& records = builder->tasks();
        for (size_t i = 0; i < records.size(); ++i) if (records[i].id == id) return records[i].kind;
        return "?";
    }
    std::string stage(const std::string& id) const {
        const std::vector<RecordingDefinitionBuilder::TaskRecord>& records = builder->tasks();
        for (size_t i = 0; i < records.size(); ++i) if (records[i].id == id) return records[i].stage;
        return "";
    }
    std::vector<std::string> prerequisites(const std::string& id) const {
        std::vector<std::string> result;
        const std::vector<RecordingDefinitionBuilder::DependencyRecord>& deps = builder->dependencies();
        for (size_t i = 0; i < deps.size(); ++i) if (deps[i].to == id) result.push_back(deps[i].from);
        return result;
    }
};

/** One running workflow instance plus its web-side bookkeeping. */
struct EngineGateway::Instance {
    std::string id;
    std::string subjectId;
    std::string subjectName;
    std::string summary;
    Industry* industry;
    std::unique_ptr<WorkflowInstance> workflow;          // real engine instance (owns the cloned tree)
    std::unique_ptr<CommandProcessor> processor;         // real engine invoker
    int commandDepth;                                    // commands currently on the processor's stack
    HistoryLog log;                                      // temporary audit trail
    SnapshotHistory snapshots;                           // real engine caretaker
    std::vector<Data> snapshotData;
    std::vector<std::string> snapshotLabels;
    std::vector<std::shared_ptr<WorkComponent> > retiredRoots;   // kept alive: the Mediator still holds their pointers
    Data data;                                           // case data (WorkflowInstance::data has no accessors)
    bool started;

    Instance() : industry(nullptr), commandDepth(0), started(false) {}
    WorkComponent* root() const { return workflow->getRoot().get(); }
};

/** RuleContext for scenario rules, bound to one instance and one action. */
class EngineGateway::Context : public RuleContext {
public:
    Context(Instance& instance, Trace& trace, WorkItem* item)
        : inst(instance), tr(trace), target(item), warning(false) {}

    WorkItem* task(const std::string& id) override { return findTask(inst.root(), id); }
    CaseData& data() override { return inst.data; }
    Trace& trace() override { return tr; }
    ResourceCoordinator& coordinator() override { return *inst.industry->coordinator; }
    ParticipantDirectory& participants() override { return *inst.industry->people; }

    Instance& inst;
    Trace& tr;
    WorkItem* target;
    std::vector<std::string> notices;
    bool warning;   // the action ran, but something the marker should notice was refused
};

// ===========================================================================
// Helpers
// ===========================================================================

namespace {

/** Captures the engine's std::cout output during one gateway call. */
class ConsoleCapture {
public:
    ConsoleCapture() : previous(std::cout.rdbuf(buffer.rdbuf())) {}
    ~ConsoleCapture() { std::cout.rdbuf(previous); }
    json lines() const {
        json out = json::array();
        std::istringstream in(buffer.str());
        std::string line;
        while (std::getline(in, line)) if (!line.empty()) out.push_back(line);
        return out;
    }

private:
    std::ostringstream buffer;
    std::streambuf* previous;
};

struct Row {
    std::string id;
    std::string name;
    std::string state;
    const Participant* assignee;
};

std::vector<Row> rows(WorkComponent* root) {
    std::vector<Row> result;
    std::vector<WorkItem*> tasks = allTasks(root);
    for (size_t i = 0; i < tasks.size(); ++i) {
        Row r = { tasks[i]->getId(), tasks[i]->getName(), stateOf(tasks[i]), tasks[i]->getAssignee() };
        result.push_back(r);
    }
    return result;
}

json traceJson(const std::vector<TraceStep>& steps) {
    json out = json::array();
    for (size_t i = 0; i < steps.size(); ++i) {
        out.push_back(json{{"actor", steps[i].actor}, {"pattern", steps[i].pattern},
                           {"detail", steps[i].detail}, {"kind", steps[i].kind}});
    }
    return out;
}

std::string join(const std::vector<std::string>& parts, const std::string& sep) {
    std::string out;
    for (size_t i = 0; i < parts.size(); ++i) out += (i ? sep : "") + parts[i];
    return out;
}

/** Which engine operation a UI action calls, for messages. */
std::string operationName(const std::string& action) {
    if (action == "assign") return "assignTo(participant)";
    return action + "()";
}

/** The concrete command the team designed for each action (for honest labelling). */
std::string designedCommand(const std::string& action) {
    if (action == "assign") return "AssignCommand";
    if (action == "complete") return "CompleteCommand";
    if (action == "approve") return "ApproveCommand";
    if (action == "reject") return "RejectCommand";
    if (action == "cancel") return "CancelCommand";
    return "";
}

void facadeStep(Trace& trace, const std::string& action) {
    if (action == "complete") {
        trace.pending("WorkflowManager::complete(itemId)", "Facade",
                      "not yet integrated: it throws 'waiting for integration'; EngineGateway::complete "
                      "does the same job (temporary)");
    } else {
        trace.pending("WorkflowManager", "Facade",
                      "not yet integrated: no " + operationName(action) +
                      " on the facade yet; EngineGateway calls the work item directly (temporary)");
    }
}

std::string nowLabel() {
    std::time_t now = std::time(nullptr);
    char buffer[16];
    std::strftime(buffer, sizeof(buffer), "%H:%M:%S", std::localtime(&now));
    return buffer;
}

} // namespace

// ===========================================================================
// Construction / reset
// ===========================================================================

EngineGateway::EngineGateway() : activeTrace(nullptr), resetCount(0) {
    ConsoleCapture quiet;
    build();
}

EngineGateway::~EngineGateway() {
    instances.clear();     // instances point into the industries, so they go first
    industries.clear();
}

void EngineGateway::build() {
    instances.clear();
    instanceOrder.clear();
    industries.clear();
    industryOrder.clear();

    std::vector<Scenario*> scenarios;
    scenarios.push_back(new EducationScenario());
    scenarios.push_back(new HealthcareScenario());

    for (size_t s = 0; s < scenarios.size(); ++s) {
        std::unique_ptr<Industry> ind(new Industry());
        ind->scenario.reset(scenarios[s]);
        Trace trace;

        ind->coordinator.reset(new ResourceCoordinator());
        ind->people.reset(new ParticipantDirectory(ind->coordinator.get()));
        ind->scenario->seedParticipants(*ind->people);
        trace.engine("Participant / Resource + ResourceCoordinator::registerColleague", "Mediator",
                     "seeded people and doctor-slot resources register with the industry's mediator");

        ind->connectors.reset(new ConnectorManager());
        ind->testConnectors["medical-aid"].reset(new TestPaymentConnector());
        ind->testConnectors["messaging"].reset(new TestMessagingConnector());
        ind->testConnectors["student-records"].reset(new TestStudentRecordsConnector());
        for (std::map<std::string, std::unique_ptr<TestConnector> >::iterator it = ind->testConnectors.begin();
             it != ind->testConnectors.end(); ++it) {
            ind->connectors->registerConnector(it->first, it->second.get());
        }
        trace.temporary("ConnectorManager (web/temporary_stubs) + Test*Connector adapters", "Adapter",
                        "stand in for Member 7's ConnectorManager and PaymentConnector/MessagingConnector/"
                        "StudentRecordsConnector");

        ind->builder.reset(new RecordingDefinitionBuilder(ind->scenario->title()));
        ind->definition = ind->scenario->buildDefinition(*ind->builder);
        trace.scenario(ind->scenario->directorName() + "::construct(builder)", "Builder (Director)",
                       "the industry's construction sequence");
        trace.temporary("RecordingDefinitionBuilder", "Builder (ConcreteBuilder)",
                        "forwards addStage/addTask to the real CodeDefinitionBuilder; records dependencies and "
                        "the approval chain, which CodeDefinitionBuilder cannot store yet");
        trace.engine("CodeDefinitionBuilder::addStage / addTask -> getDefinition()", "Builder",
                     "creates WorkStage / ManualTask / AutomatedTask / ApprovalTask and the WorkflowDefinition");

        ind->tasks = ind->scenario->tasks();
        WorkComponent* root = ind->definition->getRoot().get();
        for (size_t i = 0; i < ind->tasks.size(); ++i) {
            if (ind->tasks[i].connector.empty()) continue;
            AutomatedTask* automated = asAutomatedTask(findTask(root, ind->tasks[i].id));
            if (automated == nullptr) continue;
            automated->setConnectorName(ind->tasks[i].connector);
            automated->setConnectorManager(ind->connectors.get());
        }
        trace.engine("AutomatedTask::setConnectorName / setConnectorManager", "Adapter (client)",
                     "set once on the definition; every clone copies them");
        trace.pending("DefinitionRegistry::registerDefinition", "Prototype (PrototypeManager)",
                      "not linked: DefinitionRegistry has a header but no .cpp (Member 3); the gateway keeps the "
                      "definition itself");
        ind->buildTrace = trace.steps();

        std::string key = ind->scenario->key();
        industryOrder.push_back(key);
        industries[key] = std::move(ind);
    }
}

EngineGateway::Industry& EngineGateway::industryOrThrow(const std::string& key) {
    std::map<std::string, std::unique_ptr<Industry> >::iterator it = industries.find(key);
    if (it == industries.end()) throw GatewayError(404, "Unknown industry '" + key + "'");
    return *it->second;
}

EngineGateway::Instance& EngineGateway::instanceOrThrow(const std::string& id) {
    std::map<std::string, std::unique_ptr<Instance> >::iterator it = instances.find(id);
    if (it == instances.end()) throw GatewayError(404, "No instance '" + id + "' (was the demo reset?)");
    return *it->second;
}

json EngineGateway::reset() {
    std::lock_guard<std::mutex> lock(mutex);
    ConsoleCapture console;
    build();
    ++resetCount;
    return json{{"ok", true}, {"message", "Demo reset: both definitions rebuilt, all instances discarded."},
                {"resetCount", resetCount}};
}

// ===========================================================================
// Wiring an instance's tree to the Mediator
// ===========================================================================

void EngineGateway::wire(Instance& inst, Trace& trace) {
    Industry& ind = *inst.industry;
    WorkComponent* root = inst.root();
    std::vector<WorkItem*> tasks = allTasks(root);
    for (size_t i = 0; i < tasks.size(); ++i) ind.coordinator->registerColleague(tasks[i]);

    const std::vector<RecordingDefinitionBuilder::DependencyRecord>& deps = ind.builder->dependencies();
    for (size_t i = 0; i < deps.size(); ++i) {
        ind.coordinator->addDependency(findTask(root, deps[i].to), findTask(root, deps[i].from));
    }
    std::ostringstream detail;
    detail << tasks.size() << " tasks registered as colleagues; " << deps.size()
           << " dependencies added for THIS instance's own task objects";
    trace.temporary("RecordingDefinitionBuilder::dependencies()", "Builder",
                    "dependency list recorded at build time (CodeDefinitionBuilder::addDependency throws)");
    trace.engine("ResourceCoordinator::registerColleague / addDependency", "Mediator", detail.str());
}

void EngineGateway::updateStatus(Instance& inst) {
    if (!inst.started) {
        inst.workflow->setStatus("Created");
        return;
    }
    std::vector<WorkItem*> tasks = allTasks(inst.root());
    bool finished = true;
    for (size_t i = 0; i < tasks.size(); ++i) {
        std::string s = stateOf(tasks[i]);
        if (s != "Completed" && s != "Cancelled" && s != "Rejected") finished = false;
    }
    inst.workflow->setStatus(finished ? "Finished" : "Running");
}

// ===========================================================================
// WorkflowManager mirror
// ===========================================================================

json EngineGateway::createInstance(const std::string& industryKey, const std::string& subjectId) {
    std::lock_guard<std::mutex> lock(mutex);
    ConsoleCapture console;
    Industry& ind = industryOrThrow(industryKey);

    std::vector<SubjectSeed> subjects = ind.scenario->subjects();
    const SubjectSeed* subject = nullptr;
    for (size_t i = 0; i < subjects.size(); ++i) if (subjects[i].id == subjectId) subject = &subjects[i];
    if (subject == nullptr) throw GatewayError(400, "Pick a " + ind.scenario->subjectNoun() + " first.");

    Trace trace;
    trace.pending("WorkflowManager::createInstance(definitionName)", "Facade",
                  "not yet integrated: it throws 'waiting for integration'; EngineGateway::createInstance does "
                  "the steps below (temporary)");
    trace.pending("DefinitionRegistry::get(name)", "Prototype (PrototypeManager)",
                  "not linked (no .cpp, Member 3); the gateway holds the built definition");

    std::unique_ptr<Instance> inst(new Instance());
    inst->industry = &ind;
    std::ostringstream id;
    id << ind.scenario->idPrefix() << "-" << ++ind.counter;
    inst->id = id.str();
    inst->subjectId = subject->id;
    inst->subjectName = subject->name;
    inst->summary = subject->summary;
    inst->data = subject->data;

    std::shared_ptr<WorkComponent> root = ind.definition->cloneRoot();
    std::vector<WorkItem*> cloned = allTasks(root.get());
    std::ostringstream cloneDetail;
    cloneDetail << "deep copy of the definition: WorkStage::clone() copies each child; "
                << cloned.size() << " task clones keep their IDs but are NEW objects";
    trace.engine("WorkflowDefinition::cloneRoot()", "Prototype", cloneDetail.str());
    trace.engine("WorkStage (Composite) / ManualTask, AutomatedTask, ApprovalTask (leaves)", "Composite",
                 "the clone is a tree of stages and tasks behind the one WorkComponent interface");

    inst->workflow.reset(new WorkflowInstance(root, "Created"));
    inst->processor.reset(new CommandProcessor());
    trace.engine("new WorkflowInstance(root, \"Created\")", "Prototype (product)",
                 "independent instance: its own tree, status and history");
    trace.engine("new CommandProcessor()", "Command (Invoker)", "one undo history per instance");

    wire(*inst, trace);
    Context ctx(*inst, trace, nullptr);
    ind.scenario->configureInstance(ctx);
    updateStatus(*inst);

    inst->log.add("create", "", "", "Instance created for " + subject->name + " by cloning '" +
                  ind.definition->getName() + "'", "info");

    std::string newId = inst->id;
    instanceOrder.push_back(newId);
    instances[newId] = std::move(inst);
    Instance& created = *instances[newId];

    return json{{"ok", true}, {"message", newId + " created for " + subject->name + ". Press Start."},
                {"trace", traceJson(trace.steps())}, {"console", console.lines()},
                {"changes", json::array()}, {"instance", instanceView(created)}};
}

json EngineGateway::start(const std::string& instanceId) {
    std::lock_guard<std::mutex> lock(mutex);
    ConsoleCapture console;
    Instance& inst = instanceOrThrow(instanceId);
    Industry& ind = *inst.industry;
    Trace trace;
    trace.pending("WorkflowManager::start(instanceId)", "Facade",
                  "not yet integrated: it throws 'waiting for integration'; EngineGateway::start does the "
                  "steps below (temporary)");

    if (inst.started) {
        inst.log.add("start", "", "", "Start refused: the instance is already running", "refused");
        return json{{"ok", false}, {"message", "This instance has already been started."},
                    {"trace", traceJson(trace.steps())}, {"console", console.lines()},
                    {"changes", json::array()}, {"instance", instanceView(inst)}};
    }

    std::vector<Row> before = rows(inst.root());
    std::vector<WorkItem*> tasks = allTasks(inst.root());
    trace.engine("WorkStage::createIterator() -> DepthFirstIterator", "Iterator",
                 "walks every task of this instance's tree");
    std::vector<std::string> opened;
    for (size_t i = 0; i < tasks.size(); ++i) {
        if (ind.coordinator->canBecomeAvailable(tasks[i]) && tasks[i]->makeAvailable()) {
            opened.push_back(tasks[i]->getName());
        }
    }
    trace.engine("ResourceCoordinator::canBecomeAvailable(task)", "Mediator",
                 "true only for tasks without unfinished prerequisites");
    trace.engine("WorkItem::makeAvailable()", "State",
                 "CreatedState -> AvailableState for: " + join(opened, ", "));
    inst.started = true;
    updateStatus(inst);
    trace.engine("WorkflowInstance::setStatus(\"Running\")", "instance", "");

    json changes = json::array();
    std::vector<Row> after = rows(inst.root());
    for (size_t i = 0; i < after.size(); ++i) {
        if (after[i].state != before[i].state) {
            changes.push_back(json{{"task", after[i].id}, {"name", after[i].name},
                                   {"from", before[i].state}, {"to", after[i].state}});
        }
    }
    inst.log.add("start", "", "", "Started; available: " + join(opened, ", "), "done");
    return json{{"ok", true}, {"message", inst.id + " started. Available now: " + join(opened, ", ")},
                {"trace", traceJson(trace.steps())}, {"console", console.lines()},
                {"changes", changes}, {"instance", instanceView(inst)}};
}

json EngineGateway::complete(const std::string& i, const std::string& t, const std::string& outcome) {
    return act(i, t, "complete", "", outcome);
}

json EngineGateway::getStatus(const std::string& instanceId) {
    std::lock_guard<std::mutex> lock(mutex);
    ConsoleCapture console;
    return instanceView(instanceOrThrow(instanceId));
}

json EngineGateway::makeAvailable(const std::string& i, const std::string& t) { return act(i, t, "makeAvailable", "", ""); }
json EngineGateway::assignTo(const std::string& i, const std::string& t, const std::string& p) { return act(i, t, "assign", p, ""); }
json EngineGateway::startTask(const std::string& i, const std::string& t) { return act(i, t, "start", "", ""); }
json EngineGateway::execute(const std::string& i, const std::string& t) { return act(i, t, "execute", "", ""); }
json EngineGateway::approve(const std::string& i, const std::string& t) { return act(i, t, "approve", "", ""); }
json EngineGateway::reject(const std::string& i, const std::string& t) { return act(i, t, "reject", "", ""); }
json EngineGateway::cancel(const std::string& i, const std::string& t) { return act(i, t, "cancel", "", ""); }
json EngineGateway::escalate(const std::string& i, const std::string& t) { return act(i, t, "escalate", "", ""); }

// ===========================================================================
// One lifecycle action, wrapped in a command
// ===========================================================================

json EngineGateway::act(const std::string& instanceId, const std::string& taskId, const std::string& action,
                        const std::string& participantId, const std::string& outcome) {
    std::lock_guard<std::mutex> lock(mutex);
    ConsoleCapture console;
    Instance& inst = instanceOrThrow(instanceId);
    Industry& ind = *inst.industry;
    WorkItem* item = findTask(inst.root(), taskId);
    if (item == nullptr) throw GatewayError(404, "No task '" + taskId + "' in " + instanceId);

    Trace trace;
    Context ctx(inst, trace, item);
    facadeStep(trace, action);
    const std::string name = item->getName();
    const std::string before = stateOf(item);
    std::vector<Row> rowsBefore = rows(inst.root());

    // --- checks that refuse without running a command -------------------------------
    std::string refusal;
    if (action == "makeAvailable") {
        bool can = ind.coordinator->canBecomeAvailable(item);
        std::vector<std::string> waiting;
        std::vector<std::string> prereq = ind.prerequisites(taskId);
        for (size_t i = 0; i < prereq.size(); ++i) {
            WorkItem* p = findTask(inst.root(), prereq[i]);
            if (p != nullptr && !p->isComplete()) waiting.push_back(p->getName() + " (" + stateOf(p) + ")");
        }
        trace.engine("ResourceCoordinator::canBecomeAvailable(" + name + ")", "Mediator",
                     can ? "true: every prerequisite is complete" : "false: waiting for " + join(waiting, ", "));
        if (!can) {
            refusal = "Dependency not met: \"" + name + "\" cannot become available until " + join(waiting, ", ") +
                      " is complete. The ResourceCoordinator (Mediator) refused.";
        }
    } else if (action == "assign") {
        if (ind.people->find(participantId) == nullptr) throw GatewayError(400, "Pick a participant to assign.");
    } else if (action == "start") {
        std::string reason;
        if (!ind.scenario->canStart(ctx, taskId, reason)) {
            trace.scenario(ind.scenario->key() == "healthcare" ? "HealthcareScenario::canStart"
                                                                : "EducationScenario::canStart",
                           "industry rule", reason);
            refusal = reason;
        }
    } else if (action != "execute" && action != "complete" && action != "approve" && action != "reject" &&
               action != "cancel" && action != "escalate") {
        throw GatewayError(400, "Unknown action '" + action + "'");
    }
    if (!refusal.empty()) {
        inst.log.add(action, taskId, name, refusal, "refused");
        return json{{"ok", false}, {"message", refusal}, {"trace", traceJson(trace.steps())},
                    {"console", console.lines()}, {"changes", json::array()}, {"instance", instanceView(inst)}};
    }

    // --- run it as a command through the real CommandProcessor ----------------------
    bool executed = false;
    Data dataBefore = inst.data;
    Instance* instPtr = &inst;
    WebCommand::UndoHook hook = [this, instPtr, dataBefore]() { this->undoSideEffects(*instPtr, dataBefore); };
    WebCommand::Action run = [&]() -> bool { return this->perform(inst, ctx, action, participantId, outcome); };

    std::string designed = designedCommand(action);
    trace.temporary("WebCommand(\"" + action + "\")", "Command (ConcreteCommand)",
                    designed.empty()
                        ? "the team has no command for " + operationName(action) + " yet; this web command wraps it"
                        : "stands in for " + designed + " (Member 5), which does not compile yet");
    std::unique_ptr<WorkflowCommand> command(new WebCommand(action, inst.root(), run, &executed, hook));
    inst.processor->run(std::move(command));
    trace.engine("CommandProcessor::run(command)", "Command (Invoker)",
                 executed ? "execute() returned true -> pushed onto this instance's undo history"
                          : "execute() returned false -> not pushed (nothing to undo)");
    if (executed) ++inst.commandDepth;

    // --- report ----------------------------------------------------------------------
    std::vector<Row> rowsAfter = rows(inst.root());
    json changes = json::array();
    std::vector<std::string> changeText;
    for (size_t i = 0; i < rowsAfter.size() && i < rowsBefore.size(); ++i) {
        if (rowsAfter[i].state != rowsBefore[i].state || rowsAfter[i].assignee != rowsBefore[i].assignee) {
            changes.push_back(json{{"task", rowsAfter[i].id}, {"name", rowsAfter[i].name},
                                   {"from", rowsBefore[i].state}, {"to", rowsAfter[i].state}});
            changeText.push_back(rowsAfter[i].name + ": " + rowsBefore[i].state + " -> " + rowsAfter[i].state);
        }
    }
    updateStatus(inst);

    std::string message;
    if (executed) {
        std::string after = stateOf(item);
        message = name + ": " + operationName(action) + (before == after ? " ran (still " + after + ")"
                                                                          : " moved it " + before + " -> " + after);
        if (!ctx.notices.empty()) message += ". " + join(ctx.notices, " ");
    } else if (action == "execute") {
        message = "Not allowed: execute() only runs a task that is InProgress. \"" + name + "\" is " + before +
                  ", so WorkItem::checkReady() returned false and the Template Method stopped.";
    } else {
        message = "Not allowed: " + operationName(action) + " is not permitted while \"" + name + "\" is in state " +
                  before + ". " + before + "State refused it (returned false); nothing changed.";
    }

    HistoryEntry& entry = inst.log.add(action, taskId, name,
                                       message,
                                       executed ? "done" : "refused");
    entry.changes = changeText;
    entry.onCommandStack = executed;

    return json{{"ok", executed}, {"warning", ctx.warning}, {"message", message}, {"trace", traceJson(trace.steps())},
                {"console", console.lines()}, {"changes", changes}, {"instance", instanceView(inst)}};
}

bool EngineGateway::perform(Instance& inst, Context& ctx, const std::string& action,
                            const std::string& participantId, const std::string& outcome) {
    Industry& ind = *inst.industry;
    WorkItem* item = ctx.target;
    Trace& trace = ctx.trace();
    const std::string id = item->getId();
    const std::string name = item->getName();
    const std::string from = stateOf(item);
    ParticipantEntry* who = action == "assign" ? ind.people->find(participantId) : nullptr;

    bool ok = false;
    if (action == "makeAvailable") ok = item->makeAvailable();
    else if (action == "assign") ok = item->assignTo(PendingIntegration::chooseParticipant(item, who->participant.get(), trace));
    else if (action == "start") ok = item->start();
    else if (action == "complete") ok = item->complete();
    else if (action == "approve") ok = item->approve();
    else if (action == "reject") ok = item->reject();
    else if (action == "cancel") ok = item->cancel();
    else if (action == "escalate") ok = item->escalate();
    else if (action == "execute") ok = runTemplateMethod(inst, ctx);

    const std::string to = stateOf(item);
    if (action != "execute") {
        trace.engine(name + "." + operationName(action), "State",
                     ok ? "WorkItem delegates to " + from + "State -> transition to " + to + "State"
                        : "WorkItem delegates to " + from + "State, which returned false: not allowed in " + from);
    }
    if (!ok) return false;

    if (action == "assign" && who != nullptr) {
        trace.engine("Participant", "Mediator (colleague)", who->name + " (" + who->role + ") is now the assignee");
        Resource* resource = ind.scenario->resourceFor(id, *who);
        if (resource != nullptr) {
            ind.coordinator->release(item);
            inst.data.erase("resource." + id);
            inst.data.erase("resourceBy." + id);
            bool granted = ind.coordinator->requestResource(item, resource);
            trace.engine("ResourceCoordinator::requestResource(" + name + ", " + who->name + "'s slots)", "Mediator",
                         granted ? "GRANTED: Resource::isFree() returned true"
                                 : "REFUSED: Resource::isFree() returned false (no free slot)");
            if (granted) {
                inst.data["resource." + id] = who->name;
                inst.data["resourceBy." + id] = who->id;
                ctx.notices.push_back("Slot granted with " + who->name + ".");
            } else {
                inst.data["refused." + id] = who->name;
                ctx.warning = true;
                ctx.notices.push_back("BUT the ResourceCoordinator refused a slot with " + who->name +
                                      " (no free slot). Escalate this task, then reassign it to another GP.");
            }
        }
    }

    if (from != to) {
        ind.scenario->afterTransition(ctx, id, action, to, outcome);
        PendingIntegration::publishTransition(item, from, to, trace);
    }
    return true;
}

bool EngineGateway::runTemplateMethod(Instance& inst, Context& ctx) {
    Industry& ind = *inst.industry;
    WorkItem* item = ctx.target;
    Trace& trace = ctx.trace();
    const std::string id = item->getId();
    const std::string name = item->getName();
    const std::string from = stateOf(item);
    const std::string kind = ind.kind(id);

    bool ready = item->getState()->canExecute();
    if (!ready) {
        item->execute();   // let the engine itself refuse: checkReady() is false
        trace.engine(name + ".execute()", "Template Method",
                     "checkReady() -> " + from + "State::canExecute() is false, so the skeleton returns before "
                     "onEnter()/perform()");
        return false;
    }

    const TaskInfo* info = ind.info(id);
    TestConnector* connector = nullptr;
    if (kind == "automated" && info != nullptr && !info->connector.empty()) {
        connector = ind.testConnectors[info->connector].get();
        connector->setContext(ind.scenario->connectorContext(ctx, id));
        trace.temporary("Test adapter context", "Adapter",
                        "the stub ConnectorRequest carries no data, so the case data is handed to the adapter first");
    }

    std::vector<std::string> reached;
    if (kind == "approval") {
        Trace* t = &trace;
        ChainTrace::setSink([t, &reached](const std::string& handler, double limit, double value, bool isFinal) {
            std::ostringstream detail;
            detail << "request value " << value << " vs limit " << limit << ": "
                   << (value <= limit ? (isFinal ? "approves (end of chain)" : "approves")
                                      : (isFinal ? "REJECTS (end of chain, over limit)" : "over limit -> passes to successor"));
            t->engine((isFinal ? "FinalApproverHandler [" : "RoleApproverHandler [") + handler + "]",
                      "Chain of Responsibility", detail.str());
            reached.push_back(handler);
        });
    }

    item->execute();   // Template Method: checkReady -> onEnter -> perform -> recordResult
    ChainTrace::clearSink();
    const std::string to = stateOf(item);

    if (kind == "manual") {
        trace.engine(name + ".execute()", "Template Method",
                     "checkReady() true -> onEnter() -> ManualTask::perform() returns false (waits for its assignee) "
                     "-> recordResult(false): stays " + to + "; finish it with complete()");
    } else if (kind == "automated") {
        trace.engine(name + ".execute()", "Template Method",
                     "checkReady() true -> onEnter() -> AutomatedTask::perform() -> callConnector()");
        trace.temporary("ConnectorManager::get(\"" + (info ? info->connector : std::string()) + "\") (web/temporary_stubs)",
                        "Adapter", "stub manager until Member 7's ConnectorManager exists");
        if (connector != nullptr) {
            trace.temporary(info->connector == "medical-aid" ? "TestPaymentConnector -> " + connector->adapteeName()
                            : info->connector == "messaging" ? "TestMessagingConnector -> " + connector->adapteeName()
                                                             : "TestStudentRecordsConnector -> " + connector->adapteeName(),
                            "Adapter", connector->lastResult());
            ctx.notices.push_back(connector->lastResult() + ".");
        }
        if (to != "Completed") ctx.warning = true;
        trace.engine("WorkItem::recordResult(" + std::string(to == "Completed" ? "true" : "false") + ")",
                     "Template Method",
                     to == "Completed" ? "connector succeeded -> complete(): InProgressState -> CompletedState"
                                       : "connector failed -> stays InProgress. Exception path: escalate, then "
                                         "reassign to a person who completes it manually.");
    } else {
        trace.engine(name + ".execute()", "Template Method",
                     "checkReady() true -> onEnter() -> ApprovalTask::perform() sends an ApprovalRequest(value) to "
                     "the first handler");
        if (!reached.empty()) {
            ctx.notices.push_back("The request travelled " + join(reached, " -> ") + "; " + reached.back() +
                                  (to == "Completed" ? " approved it." : to == "Rejected" ? " rejected it." : " decided."));
        }
        if (to == "Rejected") ctx.warning = true;
        trace.engine(name + ".approve() / reject()", "State",
                     "called by the deciding handler: " + from + "State -> " + to + "State");
    }

    if (from != to) {
        ind.scenario->afterTransition(ctx, id, "execute", to, "");
        PendingIntegration::publishTransition(item, from, to, trace);
    }
    return true;
}

// ===========================================================================
// Undo (Command) and snapshots (Memento)
// ===========================================================================

void EngineGateway::undoSideEffects(Instance& inst, const Data& dataBefore) {
    Industry& ind = *inst.industry;
    Data now = inst.data;
    Data restored = dataBefore;

    // Resource allocations live in the Mediator, outside the tree, so put them back too.
    for (Data::const_iterator it = now.begin(); it != now.end(); ++it) {
        if (it->first.compare(0, 9, "resource.") != 0) continue;
        std::string task = it->first.substr(9);
        Data::const_iterator old = dataBefore.find(it->first);
        if (old == dataBefore.end() || old->second != it->second) {
            WorkItem* item = findTask(inst.root(), task);
            if (item != nullptr) ind.coordinator->release(item);
            if (activeTrace) activeTrace->engine("ResourceCoordinator::release(" + task + ")", "Mediator",
                                                 "undo hands back the slot this action obtained");
        }
    }
    for (Data::const_iterator it = dataBefore.begin(); it != dataBefore.end(); ++it) {
        if (it->first.compare(0, 9, "resource.") != 0) continue;
        std::string task = it->first.substr(9);
        Data::const_iterator current = now.find(it->first);
        if (current != now.end() && current->second == it->second) continue;
        WorkItem* item = findTask(inst.root(), task);
        ParticipantEntry* who = ind.people->find(restored["resourceBy." + task]);
        Resource* resource = who != nullptr ? ind.scenario->resourceFor(task, *who) : nullptr;
        bool granted = item != nullptr && resource != nullptr && ind.coordinator->requestResource(item, resource);
        if (!granted) {
            restored.erase(it->first);
            restored.erase("resourceBy." + task);
        }
        if (activeTrace) activeTrace->engine("ResourceCoordinator::requestResource(" + task + ")", "Mediator",
                                             granted ? "undo re-obtains the slot held before" : "undo could not re-obtain the slot");
    }
    inst.data = restored;
}

json EngineGateway::undo(const std::string& instanceId) {
    std::lock_guard<std::mutex> lock(mutex);
    ConsoleCapture console;
    Instance& inst = instanceOrThrow(instanceId);
    Trace trace;
    trace.pending("WorkflowManager", "Facade", "no undo() on the facade yet; EngineGateway::undo (temporary)");
    std::vector<Row> before = rows(inst.root());
    bool had = inst.commandDepth > 0;

    activeTrace = &trace;
    inst.processor->undo();
    activeTrace = nullptr;
    trace.engine("CommandProcessor::undo()", "Command (Invoker)",
                 had ? "popped the latest command and called its undo()"
                     : "history is empty: prints 'No commands to undo.'");

    std::string message;
    if (had) {
        --inst.commandDepth;
        trace.temporary("WebCommand::undo()", "Command (ConcreteCommand)",
                        "WorkComponent::restoreState(state, assignee) on every task - the contract's undo back door "
                        "(no hooks, no transitions)");
        HistoryEntry* undone = inst.log.markLatestUndone();
        message = "Undid " + (undone ? undone->action + " on " + (undone->taskName.empty() ? "the instance" : undone->taskName)
                                     : std::string("the last action"));
    } else {
        message = "Nothing to undo for " + inst.id + ".";
    }

    std::vector<Row> after = rows(inst.root());
    json changes = json::array();
    std::vector<std::string> changeText;
    for (size_t i = 0; i < after.size() && i < before.size(); ++i) {
        if (after[i].state != before[i].state || after[i].assignee != before[i].assignee) {
            changes.push_back(json{{"task", after[i].id}, {"name", after[i].name},
                                   {"from", before[i].state}, {"to", after[i].state}});
            changeText.push_back(after[i].name + ": " + before[i].state + " -> " + after[i].state);
        }
    }
    updateStatus(inst);
    HistoryEntry& entry = inst.log.add("undo", "", "", message, had ? "info" : "refused");
    entry.changes = changeText;
    return json{{"ok", had}, {"message", message}, {"trace", traceJson(trace.steps())},
                {"console", console.lines()}, {"changes", changes}, {"instance", instanceView(inst)}};
}

json EngineGateway::snapshot(const std::string& instanceId) {
    std::lock_guard<std::mutex> lock(mutex);
    ConsoleCapture console;
    Instance& inst = instanceOrThrow(instanceId);
    Trace trace;
    std::shared_ptr<InstanceSnapshot> snap = inst.workflow->createSnapshot();
    inst.snapshots.save(snap);
    inst.snapshotData.push_back(inst.data);
    std::ostringstream label;
    label << "#" << inst.snapshotData.size() << " at " << nowLabel() << " - " << snap->label();
    inst.snapshotLabels.push_back(label.str());
    trace.engine("WorkflowInstance::createSnapshot()", "Memento (Originator)",
                 "copies status and clones the whole tree (Prototype) into an InstanceSnapshot");
    trace.engine("SnapshotHistory::save(snapshot)", "Memento (Caretaker)", "stores it without reading it");
    trace.temporary("case-data copy", "Memento",
                    "WorkflowInstance::data has no accessors, so the gateway keeps the case data beside the snapshot");
    inst.log.add("snapshot", "", "", "Snapshot " + label.str(), "info");
    return json{{"ok", true}, {"message", "Snapshot " + label.str() + " saved."}, {"trace", traceJson(trace.steps())},
                {"console", console.lines()}, {"changes", json::array()}, {"instance", instanceView(inst)}};
}

json EngineGateway::restoreSnapshot(const std::string& instanceId) {
    std::lock_guard<std::mutex> lock(mutex);
    ConsoleCapture console;
    Instance& inst = instanceOrThrow(instanceId);
    Industry& ind = *inst.industry;
    Trace trace;

    std::shared_ptr<InstanceSnapshot> snap = inst.snapshots.latest();
    if (!snap) {
        inst.log.add("restore", "", "", "Restore refused: no snapshot taken yet", "refused");
        return json{{"ok", false}, {"message", "Take a snapshot first."}, {"trace", traceJson(trace.steps())},
                    {"console", console.lines()}, {"changes", json::array()}, {"instance", instanceView(inst)}};
    }
    trace.engine("SnapshotHistory::latest()", "Memento (Caretaker)", "hands back the newest snapshot");

    std::vector<Row> before = rows(inst.root());
    for (Data::const_iterator it = inst.data.begin(); it != inst.data.end(); ++it) {
        if (it->first.compare(0, 9, "resource.") != 0) continue;
        WorkItem* old = findTask(inst.root(), it->first.substr(9));
        if (old != nullptr) ind.coordinator->release(old);
        trace.engine("ResourceCoordinator::release(" + it->first.substr(9) + ")", "Mediator",
                     "slot allocations are outside the snapshot, so the old tree's slot is handed back");
    }

    inst.retiredRoots.push_back(inst.workflow->getRoot());
    inst.workflow->restoreSnapshot(snap);
    trace.engine("WorkflowInstance::restoreSnapshot(snapshot)", "Memento (Originator)",
                 "restores status '" + inst.workflow->getStatus() + "' and a fresh clone of the saved tree");
    wire(inst, trace);

    inst.data = inst.snapshotData.back();
    for (Data::iterator it = inst.data.begin(); it != inst.data.end();) {
        if (it->first.compare(0, 9, "resource.") != 0) { ++it; continue; }
        std::string task = it->first.substr(9);
        ParticipantEntry* who = ind.people->find(inst.data["resourceBy." + task]);
        Resource* resource = who != nullptr ? ind.scenario->resourceFor(task, *who) : nullptr;
        WorkItem* item = findTask(inst.root(), task);
        bool granted = item != nullptr && resource != nullptr && ind.coordinator->requestResource(item, resource);
        trace.engine("ResourceCoordinator::requestResource(" + task + ")", "Mediator",
                     granted ? "re-obtained the slot held when the snapshot was taken" : "slot no longer available");
        if (!granted) {
            inst.data.erase("resourceBy." + task);
            inst.data.erase(it++);
        } else {
            ++it;
        }
    }

    inst.processor.reset(new CommandProcessor());
    inst.commandDepth = 0;
    inst.log.clearCommandStack();
    trace.temporary("new CommandProcessor()", "Command",
                    "undo history cleared: the old commands point at the tree that was just replaced");
    inst.started = inst.workflow->getStatus() != "Created";
    updateStatus(inst);

    std::vector<Row> after = rows(inst.root());
    json changes = json::array();
    std::vector<std::string> changeText;
    for (size_t i = 0; i < after.size() && i < before.size(); ++i) {
        if (after[i].state != before[i].state) {
            changes.push_back(json{{"task", after[i].id}, {"name", after[i].name},
                                   {"from", before[i].state}, {"to", after[i].state}});
            changeText.push_back(after[i].name + ": " + before[i].state + " -> " + after[i].state);
        }
    }
    std::string message = "Restored " + inst.snapshotLabels.back() + ". Undo history cleared.";
    HistoryEntry& entry = inst.log.add("restore", "", "", message, "info");
    entry.changes = changeText;
    return json{{"ok", true}, {"message", message}, {"trace", traceJson(trace.steps())},
                {"console", console.lines()}, {"changes", changes}, {"instance", instanceView(inst)}};
}

// ===========================================================================
// Views
// ===========================================================================

json EngineGateway::treeNode(Industry& ind, const Data& data, WorkComponent* node, const std::vector<int>& path) {
    WorkItem* item = asWorkItem(node);
    if (item == nullptr) {
        json children = json::array();
        for (int i = 0; node->getChild(i) != nullptr; ++i) {
            std::vector<int> childPath = path;
            childPath.push_back(i);
            children.push_back(treeNode(ind, data, node->getChild(i), childPath));
        }
        return json{{"type", "stage"}, {"name", ind.builder->names().stageName(path)}, {"path", pathKey(path)},
                    {"children", children}};
    }

    const std::string id = item->getId();
    json task = {{"type", "task"}, {"id", id}, {"name", item->getName()}, {"kind", ind.kind(id)},
                 {"state", stateOf(item)}, {"canExecute", item->getState()->canExecute()},
                 {"path", pathKey(path)}};
    const ParticipantEntry* who = ind.people->byPointer(item->getAssignee());
    task["assignee"] = who ? json{{"id", who->id}, {"name", who->name}, {"role", who->role}} : json(nullptr);

    const TaskInfo* info = ind.info(id);
    if (info != nullptr) {
        task["roleHint"] = info->roleHint;
        task["connector"] = info->connector;
        task["help"] = info->help;
        task["outcomeQuestion"] = info->outcomeQuestion;
        task["outcomes"] = info->outcomes;
    }
    json prereq = json::array();
    std::vector<std::string> deps = ind.prerequisites(id);
    for (size_t i = 0; i < deps.size(); ++i) prereq.push_back(deps[i]);
    task["prerequisites"] = prereq;

    ApprovalTask* approval = asApprovalTask(item);
    if (approval != nullptr) task["requestValue"] = approval->getRequestValue();

    Data::const_iterator held = data.find("resource." + id);
    task["resourceHeldWith"] = held == data.end() ? json(nullptr) : json(held->second);
    Data::const_iterator refused = data.find("refused." + id);
    task["resourceRefusedBy"] = refused == data.end() ? json(nullptr) : json(refused->second);
    return task;
}

json EngineGateway::instanceView(Instance& inst) {
    Industry& ind = *inst.industry;
    WorkComponent* root = inst.root();

    json available = json::array();
    std::vector<WorkItem*> tasks = allTasks(root);   // DepthFirstIterator
    std::map<std::string, int> counts;
    for (size_t i = 0; i < tasks.size(); ++i) {
        std::string s = stateOf(tasks[i]);
        counts[s]++;
        if (s == "Available") available.push_back(tasks[i]->getId());
    }

    json ready = json::array();
    ReadyTaskIterator it(root);
    it.reset();
    while (it.hasNext()) {
        WorkItem* item = asWorkItem(it.next());
        if (item != nullptr) ready.push_back(item->getId());
    }

    json history = json::array();
    const std::vector<HistoryEntry>& entries = inst.log.entries();
    for (size_t i = 0; i < entries.size(); ++i) {
        history.push_back(json{{"seq", entries[i].sequence}, {"time", entries[i].time}, {"action", entries[i].action},
                               {"task", entries[i].taskId}, {"taskName", entries[i].taskName},
                               {"detail", entries[i].detail}, {"result", entries[i].result},
                               {"changes", entries[i].changes}, {"undoable", entries[i].onCommandStack}});
    }

    json data = json::object();
    for (Data::const_iterator d = inst.data.begin(); d != inst.data.end(); ++d) data[d->first] = d->second;

    return json{{"id", inst.id}, {"industry", ind.scenario->key()}, {"industryTitle", ind.scenario->title()},
                {"subject", json{{"id", inst.subjectId}, {"name", inst.subjectName}, {"summary", inst.summary},
                                 {"noun", ind.scenario->subjectNoun()}}},
                {"status", inst.workflow->getStatus()}, {"started", inst.started},
                {"outcome", data.value("outcome", "")}, {"data", data},
                {"tree", treeNode(ind, inst.data, root, std::vector<int>())},
                {"available", available}, {"readyIterator", ready}, {"counts", counts},
                {"history", history}, {"snapshots", inst.snapshotLabels}, {"canUndo", inst.commandDepth > 0},
                {"commandDepth", inst.commandDepth}};
}

json EngineGateway::summary(Instance& inst) {
    std::map<std::string, int> counts;
    std::vector<WorkItem*> tasks = allTasks(inst.root());
    for (size_t i = 0; i < tasks.size(); ++i) counts[stateOf(tasks[i])]++;
    return json{{"id", inst.id}, {"industry", inst.industry->scenario->key()}, {"subject", inst.subjectName},
                {"status", inst.workflow->getStatus()}, {"outcome", inst.data.count("outcome") ? inst.data["outcome"] : ""},
                {"counts", counts}, {"tasks", tasks.size()}};
}

json EngineGateway::listInstances() {
    std::lock_guard<std::mutex> lock(mutex);
    ConsoleCapture console;
    json list = json::array();
    for (size_t i = 0; i < instanceOrder.size(); ++i) list.push_back(summary(*instances[instanceOrder[i]]));
    return list;
}

json EngineGateway::resources(const std::string& industryKey) {
    std::lock_guard<std::mutex> lock(mutex);
    ConsoleCapture console;
    Industry& ind = industryOrThrow(industryKey);
    json out = json::array();
    const std::vector<std::unique_ptr<ParticipantEntry> >& people = ind.people->all();
    for (size_t i = 0; i < people.size(); ++i) {
        if (!people[i]->slots) continue;
        json grants = json::array();
        for (size_t k = 0; k < instanceOrder.size(); ++k) {
            Instance& inst = *instances[instanceOrder[k]];
            if (inst.industry != &ind) continue;
            for (Data::const_iterator d = inst.data.begin(); d != inst.data.end(); ++d) {
                if (d->first.compare(0, 11, "resourceBy.") == 0 && d->second == people[i]->id) {
                    grants.push_back(json{{"instance", inst.id}, {"subject", inst.subjectName},
                                          {"task", d->first.substr(11)}});
                }
            }
        }
        out.push_back(json{{"id", people[i]->id}, {"name", people[i]->name}, {"role", people[i]->role},
                           {"capacity", people[i]->slotCapacity}, {"isFree", people[i]->slots->isFree()},
                           {"grantedTo", grants}});
    }
    return out;
}

json EngineGateway::meta() {
    std::lock_guard<std::mutex> lock(mutex);
    ConsoleCapture console;
    json list = json::array();
    for (size_t i = 0; i < industryOrder.size(); ++i) {
        Industry& ind = *industries[industryOrder[i]];
        json subjects = json::array();
        std::vector<SubjectSeed> seeds = ind.scenario->subjects();
        for (size_t k = 0; k < seeds.size(); ++k) {
            subjects.push_back(json{{"id", seeds[k].id}, {"name", seeds[k].name}, {"summary", seeds[k].summary}});
        }
        json people = json::array();
        const std::vector<std::unique_ptr<ParticipantEntry> >& entries = ind.people->all();
        for (size_t k = 0; k < entries.size(); ++k) {
            people.push_back(json{{"id", entries[k]->id}, {"name", entries[k]->name}, {"role", entries[k]->role},
                                  {"slots", entries[k]->slotCapacity}});
        }
        json chain = json::array();
        std::vector<ChainLink> links = ind.scenario->chain();
        for (size_t k = 0; k < links.size(); ++k) {
            chain.push_back(json{{"role", links[k].role}, {"limit", links[k].limit}, {"final", links[k].isFinal}});
        }
        json pending = json::array();
        const std::vector<RecordingDefinitionBuilder::PendingStep>& steps = ind.builder->pendingSteps();
        for (size_t k = 0; k < steps.size(); ++k) {
            pending.push_back(json{{"step", steps[k].step}, {"task", steps[k].task}, {"detail", steps[k].detail}});
        }
        list.push_back(json{{"key", ind.scenario->key()}, {"title", ind.scenario->title()},
                            {"description", ind.scenario->description()}, {"noun", ind.scenario->subjectNoun()},
                            {"director", ind.scenario->directorName()},
                            {"definitionName", ind.definition->getName()},
                            {"subjects", subjects}, {"participants", people}, {"chain", chain},
                            {"chainTask", ind.scenario->chainTaskId()},
                            {"definition", treeNode(ind, Data(), ind.definition->getRoot().get(), std::vector<int>())},
                            {"buildTrace", traceJson(ind.buildTrace)}, {"pendingBuilderSteps", pending}});
    }
    return json{{"industries", list}, {"resetCount", resetCount},
                {"observer", PendingIntegration::observerStatus()},
                {"strategy", PendingIntegration::strategyStatus()}};
}
