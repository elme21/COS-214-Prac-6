#include "EngineGateway.h"

#include "Trace.h"
#include "PendingIntegration.h"

// Industry-specific code (web/scenarios/)
#include "Scenario.h"
#include "EducationScenario.h"
#include "HealthcareScenario.h"
#include "Staff.h"
#include "StageNameRecorder.h"
#include "TaskLookup.h"
#include "ChainTrace.h"
#include "RuleHelpers.h"

// The real TIVIDY engine (include/). WorkComponent.h must come before ApproveCommand.h,
// which uses WorkComponent / WorkItemState / Participant without declaring them.
#include "WorkComponent.h"
#include "WorkItem.h"
#include "WorkItemState.h"
#include "AutomatedTask.h"
#include "ApprovalTask.h"
#include "CodeDefinitionBuilder.h"
#include "DefinitionRegistry.h"
#include "WorkflowDefinition.h"
#include "WorkflowInstance.h"
#include "InstanceSnapshot.h"
#include "SnapshotHistory.h"
#include "CommandProcessor.h"
#include "WorkflowCommand.h"
#include "MakeAvailableCommand.h"
#include "AssignCommand.h"
#include "StartCommand.h"
#include "CompleteCommand.h"
#include "ApproveCommand.h"
#include "RejectCommand.h"
#include "CancelCommand.h"
#include "EscalateCommand.h"
#include "ReadyTaskIterator.h"
#include "ResourceCoordinator.h"
#include "Resource.h"
#include "Participant.h"
#include "Event.h"
#include "WorkflowEventBus.h"
#include "DependencyActivator.h"
#include "AuditLogger.h"
#include "NotificationSender.h"
#include "MonitoringCollector.h"

#include <ctime>
#include <iostream>
#include <sstream>

typedef std::map<std::string, std::string> Data;
using nlohmann::json;

// ===========================================================================
// Internal records
// ===========================================================================

/** Everything one industry needs, rebuilt from scratch by reset(). */
struct EngineGateway::Industry {
    std::unique_ptr<Scenario> scenario;
    std::unique_ptr<Staff> staff;                        // real Participant / Resource objects
    std::unique_ptr<CodeDefinitionBuilder> builder;      // real ConcreteBuilder
    std::unique_ptr<StageNameRecorder> recorder;         // temporary: stage names only
    const WorkflowDefinition* definition;                // owned by the real DefinitionRegistry
    std::vector<TaskInfo> tasks;
    std::vector<TraceStep> buildTrace;
    int counter;

    Industry() : definition(nullptr), counter(0) {}

    const TaskInfo* info(const std::string& id) const {
        for (size_t i = 0; i < tasks.size(); ++i) if (tasks[i].id == id) return &tasks[i];
        return nullptr;
    }
    std::vector<std::string> prerequisites(const std::string& id) const {
        std::vector<std::string> result;
        const std::vector<std::pair<std::string, std::string> >& deps = definition->getDependencies();
        for (size_t i = 0; i < deps.size(); ++i) if (deps[i].second == id) result.push_back(deps[i].first);
        return result;
    }
};

/** One running workflow instance: real engine objects only, plus display bookkeeping. */
struct EngineGateway::Instance {
    std::string id;
    std::string subjectId;
    std::string subjectName;
    std::string summary;
    Industry* industry;

    std::unique_ptr<WorkflowInstance> workflow;          // real Originator, owns the cloned tree
    std::unique_ptr<ResourceCoordinator> mediator;       // real Mediator for this instance
    std::unique_ptr<CommandProcessor> commands;          // real Invoker with its own history
    std::vector<std::string> commandLabels;              // display text for getHistory(), same order

    std::unique_ptr<WorkflowEventBus> bus;               // real Subject (declared before its listeners)
    std::unique_ptr<DependencyActivator> activator;      // real listeners
    std::unique_ptr<AuditLogger> audit;
    std::unique_ptr<NotificationSender> notifier;
    std::unique_ptr<MonitoringCollector> monitor;

    SnapshotHistory snapshots;                           // real Caretaker
    std::vector<Data> snapshotData;                      // temporary: InstanceSnapshot omits instance data
    std::vector<std::string> snapshotLabels;

    // Replaced by a snapshot restore but kept alive: nothing may dangle.
    std::vector<std::shared_ptr<WorkComponent> > retiredRoots;
    std::vector<std::unique_ptr<ResourceCoordinator> > retiredMediators;

    Data data;                                           // working copy; stored with WorkflowInstance::setData
    bool started;

    Instance() : industry(nullptr), started(false) {}
    WorkComponent* root() const { return workflow->getRoot().get(); }
};

/** State of one task at a moment, to find what an action changed. */
struct EngineGateway::Row {
    std::string id;
    std::string name;
    std::string state;
    const Participant* assignee;
};

/** RuleContext for scenario rules, bound to one instance and one action. */
class EngineGateway::Context : public RuleContext {
public:
    Context(EngineGateway& gateway, Instance& instance, Trace& trace)
        : owner(gateway), inst(instance), tr(trace), warning(false) {}

    TaskRef task(const std::string& id) override { return findTask(inst.root(), id); }
    bool run(WorkflowCommand* command, const std::string& label) override {
        return owner.runCommand(inst, command, label);
    }
    CaseData& data() override { return inst.data; }
    Trace& trace() override { return tr; }
    ResourceCoordinator& coordinator() override { return *inst.mediator; }
    Staff& staff() override { return *inst.industry->staff; }

    EngineGateway& owner;
    Instance& inst;
    Trace& tr;
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

std::string operationName(const std::string& action) {
    return action == "assign" ? std::string("assignTo(participant)") : action + "()";
}

std::string kindOf(const TaskRef& ref) {
    if (asApprovalTask(ref) != nullptr) return "approval";
    if (asAutomatedTask(ref) != nullptr) return "automated";
    return "manual";
}

/** Which Observer event a new state is announced as (table lookup, no switch). */
EventType eventFor(const std::string& state) {
    static std::map<std::string, EventType> table;
    if (table.empty()) {
        table["Available"] = EventType::ITEM_AVAILABLE;
        table["Assigned"] = EventType::ITEM_ASSIGNED;
        table["Completed"] = EventType::ITEM_COMPLETED;
        table["Rejected"] = EventType::ITEM_REJECTED;
        table["Cancelled"] = EventType::ITEM_CANCELLED;
    }
    std::map<std::string, EventType>::const_iterator it = table.find(state);
    return it == table.end() ? EventType::EXTERNAL_EVENT : it->second;
}

std::string nowLabel() {
    std::time_t now = std::time(nullptr);
    char buffer[16];
    std::strftime(buffer, sizeof(buffer), "%H:%M:%S", std::localtime(&now));
    return buffer;
}

void facadeStep(Trace& trace, const std::string& operation) {
    trace.pending("WorkflowManager::" + operation, "Facade",
                  "not integrated: WorkflowManager does not link (it needs TaskRouter, which does not compile) "
                  "and gives no access to an instance's tasks, so EngineGateway calls the engine directly");
}

} // namespace

// ===========================================================================
// Construction / reset
// ===========================================================================

EngineGateway::EngineGateway() : resetCount(0) {
    ConsoleCapture quiet;
    build();
}

EngineGateway::~EngineGateway() {
    instances.clear();     // instances point into the industries and the registry, so they go first
    industries.clear();
    registry.reset();
}

void EngineGateway::build() {
    instances.clear();
    instanceOrder.clear();
    industries.clear();
    industryOrder.clear();
    registry.reset(new DefinitionRegistry());

    std::vector<Scenario*> scenarios;
    scenarios.push_back(new EducationScenario());
    scenarios.push_back(new HealthcareScenario());

    for (size_t s = 0; s < scenarios.size(); ++s) {
        std::unique_ptr<Industry> ind(new Industry());
        ind->scenario.reset(scenarios[s]);
        Trace trace;

        ind->staff.reset(new Staff());
        ind->scenario->seedStaff(*ind->staff);
        trace.engine("new Participant(id, name, role) / new Resource(name, capacity)", "Mediator (colleagues)",
                     "seeded people; GPs get a Resource whose places are taken and given back by the coordinator");

        ind->builder.reset(new CodeDefinitionBuilder(ind->scenario->definitionName()));
        ind->recorder.reset(new StageNameRecorder(*ind->builder));
        ind->scenario->construct(*ind->recorder);
        trace.scenario(ind->scenario->directorName() + "::construct(builder)", "Builder (Director)",
                       "the industry's construction sequence");
        trace.engine("CodeDefinitionBuilder::addStage / addTask / addDependency / setApprovalChain / addDecorator",
                     "Builder (ConcreteBuilder)",
                     "real builder: tasks, dependencies, approval chain and an AuditDecorator on " +
                         join(ind->scenario->decoratedTasks(), ", "));
        trace.temporary("StageNameRecorder", "Builder",
                        "forwards every step unchanged; only notes stage names, because WorkStage has no name");

        std::shared_ptr<WorkflowDefinition> built = ind->recorder->getDefinition();
        ind->tasks = ind->scenario->tasks();
        std::vector<TaskRef> tasks = allTasks(built->getRoot().get());
        for (size_t i = 0; i < tasks.size(); ++i) {
            const TaskInfo* info = ind->info(tasks[i].item->getId());
            if (info != nullptr) tasks[i].item->setName(info->label);   // display label; the ID stays
        }

        // The registry takes ownership of a raw pointer, but the builder returns a shared_ptr,
        // so the registry gets its own copy (it shares the same prototype root).
        registry->registerDefinition(new WorkflowDefinition(*built));
        ind->definition = registry->get(ind->scenario->definitionName());
        trace.engine("DefinitionRegistry::registerDefinition / get", "Prototype (PrototypeManager)",
                     "both industries' definitions live in one registry; instances clone from it");
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
// Engine plumbing
// ===========================================================================

bool EngineGateway::runCommand(Instance& inst, WorkflowCommand* command, const std::string& label) {
    bool ok = inst.commands->run(std::unique_ptr<WorkflowCommand>(command));
    if (ok) inst.commandLabels.push_back(label);
    return ok;
}

void EngineGateway::wire(Instance& inst, Trace& trace) {
    WorkComponent* root = inst.root();
    std::vector<TaskRef> tasks = allTasks(root);
    for (size_t i = 0; i < tasks.size(); ++i) inst.mediator->registerColleague(tasks[i].item);

    const std::vector<std::pair<std::string, std::string> >& deps = inst.industry->definition->getDependencies();
    for (size_t i = 0; i < deps.size(); ++i) {
        inst.mediator->addDependency(findTask(root, deps[i].second).item, findTask(root, deps[i].first).item);
    }
    std::ostringstream detail;
    detail << tasks.size() << " tasks registered; " << deps.size()
           << " dependencies from WorkflowDefinition::getDependencies() added for THIS instance's task objects";
    trace.engine("ResourceCoordinator::registerColleague / addDependency", "Mediator", detail.str());
}

void EngineGateway::publishChanges(Instance& inst, std::vector<Row>& seen, Trace& trace, bool fromUndo) {
    // Announce every task whose state changed, one at a time: publishing ITEM_COMPLETED lets the
    // real DependencyActivator unlock dependants, which are then announced in turn.
    for (int guard = 0; guard < 200; ++guard) {
        std::vector<TaskRef> tasks = allTasks(inst.root());
        size_t i = 0;
        for (; i < tasks.size() && i < seen.size(); ++i) {
            if (stateOf(tasks[i].node) != seen[i].state) break;
        }
        if (i >= tasks.size() || i >= seen.size()) return;

        std::string from = seen[i].state;
        std::string to = stateOf(tasks[i].node);
        seen[i].state = to;
        seen[i].assignee = tasks[i].node->getAssignee();

        EventType type = fromUndo ? EventType::EXTERNAL_EVENT : eventFor(to);
        std::string detail = (fromUndo ? "undo: " : "") + from + " -> " + to;
        inst.bus->publish(Event(type, inst.id, tasks[i].item->getId(), detail, tasks[i].item));
        trace.engine("WorkflowEventBus::publish(" + eventTypeToString(type) + ", " + tasks[i].item->getName() + ")",
                     "Observer (Subject)", detail + " -> AuditLogger, NotificationSender, MonitoringCollector notified");
        if (type == EventType::ITEM_COMPLETED) {
            trace.engine("DependencyActivator::update()", "Observer -> Mediator",
                         "calls Colleague::changed(\"completed\"): ResourceCoordinator::notifyChanged asks each "
                         "dependant canBecomeAvailable() and makes it available");
        }
    }
}

void EngineGateway::updateStatus(Instance& inst) {
    if (!inst.started) {
        inst.workflow->setStatus("Created");
    } else {
        std::vector<TaskRef> tasks = allTasks(inst.root());
        bool finished = true;
        for (size_t i = 0; i < tasks.size(); ++i) {
            std::string s = stateOf(tasks[i].node);
            if (s != "Completed" && s != "Cancelled" && s != "Rejected") finished = false;
        }
        inst.workflow->setStatus(finished ? "Finished" : "Running");
    }
    inst.workflow->setData(inst.data);
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
    facadeStep(trace, "createInstance(definitionName)");

    std::unique_ptr<Instance> inst(new Instance());
    inst->industry = &ind;
    std::ostringstream id;
    id << ind.scenario->idPrefix() << "-" << ++ind.counter;
    inst->id = id.str();
    inst->subjectId = subject->id;
    inst->subjectName = subject->name;
    inst->summary = subject->summary;
    inst->data = subject->data;

    const WorkflowDefinition* def = registry->get(ind.scenario->definitionName());
    trace.engine("DefinitionRegistry::get(\"" + def->getName() + "\")", "Prototype (PrototypeManager)", "");
    std::shared_ptr<WorkComponent> root = def->cloneRoot();
    std::ostringstream cloneDetail;
    cloneDetail << "deep copy: WorkStage::clone() copies each child, decorators clone what they wrap; "
                << allTasks(root.get()).size() << " NEW task objects with the same IDs";
    trace.engine("WorkflowDefinition::cloneRoot()", "Prototype", cloneDetail.str());

    inst->workflow.reset(new WorkflowInstance(root, "Created"));
    inst->commands.reset(new CommandProcessor());
    inst->mediator.reset(new ResourceCoordinator());
    trace.engine("new WorkflowInstance / CommandProcessor / ResourceCoordinator", "Memento / Command / Mediator",
                 "each instance gets its own tree, undo history and mediator");

    inst->bus.reset(new WorkflowEventBus());
    inst->activator.reset(new DependencyActivator(inst->bus.get()));
    inst->audit.reset(new AuditLogger(inst->bus.get()));
    inst->notifier.reset(new NotificationSender(inst->bus.get()));
    inst->monitor.reset(new MonitoringCollector(inst->bus.get()));
    trace.engine("new WorkflowEventBus + DependencyActivator, AuditLogger, NotificationSender, MonitoringCollector",
                 "Observer", "listeners subscribe to the instance's own event bus");

    wire(*inst, trace);
    Context ctx(*this, *inst, trace);
    ind.scenario->configureInstance(ctx);
    updateStatus(*inst);
    inst->bus->publish(Event(EventType::INSTANCE_CREATED, inst->id, "", "for " + subject->name));
    trace.engine("WorkflowEventBus::publish(INSTANCE_CREATED)", "Observer (Subject)", "");

    std::string newId = inst->id;
    instanceOrder.push_back(newId);
    instances[newId] = std::move(inst);
    Instance& created = *instances[newId];
    std::vector<Row> none = rowsOf(created.root());
    return respond(created, true, newId + " created for " + subject->name + ". Press Start.", false, trace, none,
                   console.lines());
}

json EngineGateway::start(const std::string& instanceId) {
    std::lock_guard<std::mutex> lock(mutex);
    ConsoleCapture console;
    Instance& inst = instanceOrThrow(instanceId);
    Trace trace;
    facadeStep(trace, "start(instanceId)");
    std::vector<Row> before = rowsOf(inst.root());

    if (inst.started) {
        return respond(inst, false, "This instance has already been started.", false, trace, before, console.lines());
    }

    std::vector<std::string> opened;
    std::vector<TaskRef> tasks = allTasks(inst.root());
    for (size_t i = 0; i < tasks.size(); ++i) {
        // Same steps as WorkflowManager::start(): mediator check, then makeAvailable().
        if (inst.mediator->canBecomeAvailable(tasks[i].item) && tasks[i].node->makeAvailable()) {
            opened.push_back(tasks[i].item->getName());
        }
    }
    trace.engine("WorkStage::createIterator() -> DepthFirstIterator", "Iterator", "walks every task of this instance");
    trace.engine("ResourceCoordinator::canBecomeAvailable -> WorkComponent::makeAvailable()", "Mediator + State",
                 "CreatedState -> AvailableState for: " + join(opened, ", "));
    inst.started = true;
    updateStatus(inst);
    inst.bus->publish(Event(EventType::INSTANCE_STARTED, inst.id));
    trace.engine("WorkflowEventBus::publish(INSTANCE_STARTED)", "Observer (Subject)", "");
    std::vector<Row> seen = before;
    publishChanges(inst, seen, trace, false);
    return respond(inst, true, inst.id + " started. Available now: " + join(opened, ", "), false, trace, before,
                   console.lines());
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
// One lifecycle action
// ===========================================================================

namespace {

/** The real command for each action (table lookup, no switch). */
WorkflowCommand* commandFor(const std::string& action, WorkComponent* node, Participant* participant,
                            std::string& className) {
    if (action == "makeAvailable") { className = "MakeAvailableCommand"; return new MakeAvailableCommand(node); }
    if (action == "assign") { className = "AssignCommand"; return new AssignCommand(node, participant); }
    if (action == "start") { className = "StartCommand"; return new StartCommand(node); }
    if (action == "complete") { className = "CompleteCommand"; return new CompleteCommand(node); }
    if (action == "approve") { className = "ApproveCommand"; return new ApproveCommand(node); }
    if (action == "reject") { className = "RejectCommand"; return new RejectCommand(node); }
    if (action == "cancel") { className = "CancelCommand"; return new CancelCommand(node); }
    if (action == "escalate") { className = "EscalateCommand"; return new EscalateCommand(node); }
    return nullptr;
}

bool isAction(const std::string& action) {
    static const char* names[] = { "makeAvailable", "assign", "start", "execute", "complete", "approve",
                                   "reject", "cancel", "escalate" };
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i) if (action == names[i]) return true;
    return false;
}

} // namespace

std::vector<EngineGateway::Row> EngineGateway::rowsOf(WorkComponent* root) {
    std::vector<Row> result;
    std::vector<TaskRef> tasks = allTasks(root);
    for (size_t i = 0; i < tasks.size(); ++i) {
        Row r = { tasks[i].item->getId(), tasks[i].item->getName(), stateOf(tasks[i].node),
                  tasks[i].node->getAssignee() };
        result.push_back(r);
    }
    return result;
}

json EngineGateway::act(const std::string& instanceId, const std::string& taskId, const std::string& action,
                        const std::string& participantId, const std::string& outcome) {
    std::lock_guard<std::mutex> lock(mutex);
    ConsoleCapture console;
    Instance& inst = instanceOrThrow(instanceId);
    Industry& ind = *inst.industry;
    TaskRef task = findTask(inst.root(), taskId);
    if (task.node == nullptr) throw GatewayError(404, "No task '" + taskId + "' in " + instanceId);
    if (!isAction(action)) throw GatewayError(400, "Unknown action '" + action + "'");

    Trace trace;
    Context ctx(*this, inst, trace);
    facadeStep(trace, action == "complete" ? "complete(instanceId, taskId)" : operationName(action));
    const std::string name = task.item->getName();
    const std::string before = stateOf(task.node);
    std::vector<Row> rowsBefore = rowsOf(inst.root());

    // --- checks that refuse without running a command -------------------------------
    std::string refusal;
    if (action == "makeAvailable") {
        bool can = inst.mediator->canBecomeAvailable(task.item);
        std::vector<std::string> waiting;
        std::vector<std::string> prereq = ind.prerequisites(taskId);
        for (size_t i = 0; i < prereq.size(); ++i) {
            TaskRef p = findTask(inst.root(), prereq[i]);
            if (p.item != nullptr && !p.item->isComplete()) waiting.push_back(p.item->getName() + " (" + stateOf(p.node) + ")");
        }
        trace.engine("ResourceCoordinator::canBecomeAvailable(" + name + ")", "Mediator",
                     can ? "true: every prerequisite is complete" : "false: waiting for " + join(waiting, ", "));
        if (!can) {
            refusal = "Dependency not met: \"" + name + "\" cannot become available until " + join(waiting, ", ") +
                      " is complete. The ResourceCoordinator (Mediator) refused.";
        }
    } else if (action == "assign") {
        if (ind.staff->find(participantId) == nullptr) throw GatewayError(400, "Pick a participant to assign.");
    } else if (action == "start") {
        std::string reason;
        if (!ind.scenario->canStart(ctx, taskId, reason)) {
            trace.scenario("HealthcareScenario::canStart", "industry rule", reason);
            refusal = reason;
        }
    }
    if (!refusal.empty()) return respond(inst, false, refusal, false, trace, rowsBefore, console.lines());

    bool ok = perform(inst, ctx, task, action, participantId, outcome);

    std::vector<Row> seen = rowsBefore;
    if (ok) publishChanges(inst, seen, trace, false);
    updateStatus(inst);

    // Decorator effect: the real AuditDecorator writes one line per attempt to its sink (std::cout).
    json consoleLines = console.lines();
    for (size_t i = 0; i < consoleLines.size(); ++i) {
        std::string line = consoleLines[i];
        if (line.compare(0, taskId.size() + 2, taskId + ": ") == 0) {
            trace.engine("AuditDecorator (on " + name + ")", "Decorator", "audit line: \"" + line + "\"");
        }
    }

    std::string message;
    if (ok) {
        std::string after = stateOf(task.node);
        message = name + ": " + operationName(action) + (before == after ? " ran (still " + after + ")"
                                                                          : " moved it " + before + " -> " + after);
        if (!ctx.notices.empty()) message += ". " + join(ctx.notices, " ");
    } else if (action == "execute") {
        message = "Not allowed: execute() only runs a task that is InProgress. \"" + name + "\" is " + before +
                  ", so WorkItem::checkReady() returned false and the Template Method stopped.";
    } else {
        message = "Not allowed: " + operationName(action) + " is not permitted while \"" + name + "\" is in state " +
                  before + ". The engine refused it (returned false); nothing changed.";
    }
    return respond(inst, ok, message, ctx.warning, trace, rowsBefore, consoleLines);
}

bool EngineGateway::perform(Instance& inst, Context& ctx, const TaskRef& task, const std::string& action,
                            const std::string& participantId, const std::string& outcome) {
    Industry& ind = *inst.industry;
    Trace& trace = ctx.trace();
    const std::string id = task.item->getId();
    const std::string name = task.item->getName();
    const std::string from = stateOf(task.node);
    Participant* who = action == "assign" ? ind.staff->find(participantId) : nullptr;

    bool ok = false;
    if (action == "execute") {
        ok = runTemplateMethod(inst, ctx, task);
    } else {
        if (action == "assign") who = PendingIntegration::chooseParticipant(task.item, who, trace);
        std::string className;
        WorkflowCommand* command = commandFor(action, task.node, who, className);
        ok = runCommand(inst, command, className + "(" + id + ")");
        std::string to = stateOf(task.node);
        trace.engine(className + "(" + name + ") -> CommandProcessor::run()", "Command",
                     ok ? "execute() returned true: pushed onto this instance's history (getHistory())"
                        : "execute() returned false: not pushed, nothing to undo");
        if (!task.decorators.empty()) {
            trace.engine(join(task.decorators, " -> ") + " -> " + kindOf(task) + " task", "Decorator",
                         "the command acts on the outermost decorator, which forwards to the task");
        }
        trace.engine(name + "." + operationName(action), "State",
                     ok ? "WorkItem delegates to " + from + "State -> transition to " + to + "State"
                        : "WorkItem delegates to " + from + "State, which returned false: not allowed in " + from);
    }
    if (!ok) return false;

    const std::string to = stateOf(task.node);
    if (action == "assign" && who != nullptr) {
        trace.engine("Participant " + who->getName(), "Mediator (colleague)",
                     who->getRole() + " is now the assignee");
        Resource* resource = ind.scenario->resourceFor(id, participantId, *ind.staff);
        if (resource != nullptr) {
            inst.mediator->release(task.item);   // hands back a slot held from an earlier assignment
            inst.data.erase("resource." + id);
            inst.data.erase("resourceBy." + id);
            bool granted = inst.mediator->requestResource(task.item, resource);
            std::ostringstream places;
            places << resource->getAvailable() << " of " << resource->getCapacity() << " places now free";
            trace.engine("ResourceCoordinator::requestResource(" + name + ", " + resource->getName() + ")", "Mediator",
                         granted ? "GRANTED: Resource::take() - " + places.str()
                                 : "REFUSED: Resource::isFree() is false - " + places.str());
            if (granted) {
                inst.data["resource." + id] = who->getName();
                inst.data["resourceBy." + id] = who->getID();
                inst.data.erase("refused." + id);
                ctx.notices.push_back("Slot granted with " + who->getName() + " (" + places.str() + ").");
            } else {
                inst.data["refused." + id] = who->getName();
                ctx.warning = true;
                ctx.notices.push_back("BUT the ResourceCoordinator refused a slot with " + who->getName() +
                                      " (no free place). Escalate this task, then reassign it to another GP.");
            }
        }
    }

    if (from != to) ind.scenario->afterTransition(ctx, id, to, outcome);
    return true;
}

bool EngineGateway::runTemplateMethod(Instance& inst, Context& ctx, const TaskRef& task) {
    Trace& trace = ctx.trace();
    const std::string name = task.item->getName();
    const std::string from = stateOf(task.node);
    const std::string kind = kindOf(task);

    trace.pending("ExecuteCommand", "Command",
                  "the engine has no command for execute(), so it is called directly and cannot be undone");
    bool ready = task.node->getState()->canExecute();
    if (!ready) {
        task.node->execute();   // let the engine itself refuse: checkReady() is false
        trace.engine(name + ".execute()", "Template Method",
                     "checkReady() -> " + from + "State::canExecute() is false, so the skeleton returns before "
                     "onEnter()/perform()");
        return false;
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

    task.node->execute();   // Template Method: checkReady -> onEnter -> perform -> recordResult
    ChainTrace::clearSink();
    const std::string to = stateOf(task.node);

    if (kind == "manual") {
        trace.engine(name + ".execute()", "Template Method",
                     "checkReady() true -> onEnter() -> ManualTask::perform() returns false (waits for its assignee) "
                     "-> recordResult(false): stays " + to + "; finish it with complete()");
    } else if (kind == "automated") {
        bool succeeds = asAutomatedTask(task)->getStepSucceeds();
        trace.engine(name + ".execute()", "Template Method",
                     std::string("checkReady() true -> onEnter() -> AutomatedTask::perform() returns stepSucceeds = ") +
                         (succeeds ? "true -> recordResult(true) -> complete(): InProgressState -> CompletedState"
                                   : "false -> recordResult(false): stays InProgress. Exception path: escalate, "
                                     "then reassign to a person who completes it by hand."));
        ctx.notices.push_back(succeeds ? "Automated step succeeded." : "Automated step FAILED: the task stays InProgress.");
        if (!succeeds) ctx.warning = true;
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
                     "called by the deciding handler on the task itself (bypassing its decorators): " + from +
                         "State -> " + to + "State");
    }

    if (from != to) inst.industry->scenario->afterTransition(ctx, task.item->getId(), to, "");
    return true;
}

// ===========================================================================
// Undo (Command) and snapshots (Memento)
// ===========================================================================

json EngineGateway::undo(const std::string& instanceId) {
    std::lock_guard<std::mutex> lock(mutex);
    ConsoleCapture console;
    Instance& inst = instanceOrThrow(instanceId);
    Trace trace;
    facadeStep(trace, "undo (not offered by the facade)");
    std::vector<Row> before = rowsOf(inst.root());
    bool had = !inst.commands->getHistory().empty();
    std::string label = had && !inst.commandLabels.empty() ? inst.commandLabels.back() : std::string();

    inst.commands->undo();
    if (had && !inst.commandLabels.empty()) inst.commandLabels.pop_back();
    trace.engine("CommandProcessor::undo()", "Command (Invoker)",
                 had ? "popped " + label + " and called its undo()" : "history is empty: nothing to undo");
    if (had) {
        trace.engine(label + ".undo()", "Command (ConcreteCommand)",
                     "WorkComponent::restoreState(previous state, previous assignee) on THIS task only; "
                     "effects on other tasks or slots are not undone by the engine's commands");
    }
    std::vector<Row> seen = before;
    publishChanges(inst, seen, trace, true);
    updateStatus(inst);
    std::string message = had ? "Undid " + label + "." : "Nothing to undo for " + inst.id + ".";
    return respond(inst, had, message, false, trace, before, console.lines());
}

json EngineGateway::snapshot(const std::string& instanceId) {
    std::lock_guard<std::mutex> lock(mutex);
    ConsoleCapture console;
    Instance& inst = instanceOrThrow(instanceId);
    Trace trace;
    std::vector<Row> before = rowsOf(inst.root());
    inst.workflow->setData(inst.data);
    std::shared_ptr<InstanceSnapshot> snap = inst.workflow->createSnapshot();
    inst.snapshots.save(snap);
    inst.snapshotData.push_back(inst.data);
    std::ostringstream label;
    label << "#" << inst.snapshotData.size() << " at " << nowLabel() << " - " << snap->label();
    inst.snapshotLabels.push_back(label.str());
    trace.engine("WorkflowInstance::createSnapshot()", "Memento (Originator)",
                 "copies the status and clones the whole tree (Prototype) into an InstanceSnapshot");
    trace.engine("SnapshotHistory::save(snapshot)", "Memento (Caretaker)", "stores it without reading it");
    trace.temporary("case-data copy", "Memento",
                    "InstanceSnapshot saves status and tree but not WorkflowInstance::getData(), so the gateway keeps "
                    "a copy beside it");
    inst.bus->publish(Event(EventType::EXTERNAL_EVENT, inst.id, "", "snapshot " + label.str()));
    return respond(inst, true, "Snapshot " + label.str() + " saved.", false, trace, before, console.lines());
}

json EngineGateway::restoreSnapshot(const std::string& instanceId) {
    std::lock_guard<std::mutex> lock(mutex);
    ConsoleCapture console;
    Instance& inst = instanceOrThrow(instanceId);
    Industry& ind = *inst.industry;
    Trace trace;
    std::vector<Row> before = rowsOf(inst.root());

    std::shared_ptr<InstanceSnapshot> snap = inst.snapshots.latest();
    if (!snap) return respond(inst, false, "Take a snapshot first.", false, trace, before, console.lines());
    trace.engine("SnapshotHistory::latest()", "Memento (Caretaker)", "hands back the newest snapshot");

    // Slots are held in the Mediator, outside the snapshot: hand back the old tree's places.
    std::vector<TaskRef> oldTasks = allTasks(inst.root());
    for (size_t i = 0; i < oldTasks.size(); ++i) inst.mediator->release(oldTasks[i].item);
    trace.engine("ResourceCoordinator::release(every old task)", "Mediator", "the replaced tree gives its slots back");

    inst.retiredRoots.push_back(inst.workflow->getRoot());
    inst.retiredMediators.push_back(std::move(inst.mediator));
    inst.workflow->restoreSnapshot(snap);
    trace.engine("WorkflowInstance::restoreSnapshot(snapshot)", "Memento (Originator)",
                 "restores status '" + inst.workflow->getStatus() + "' and a fresh clone of the saved tree");
    inst.mediator.reset(new ResourceCoordinator());
    wire(inst, trace);

    inst.data = inst.snapshotData.back();
    for (Data::iterator it = inst.data.begin(); it != inst.data.end();) {
        if (it->first.compare(0, 9, "resource.") != 0) { ++it; continue; }
        std::string taskId = it->first.substr(9);
        std::string personId = inst.data["resourceBy." + taskId];
        Resource* resource = ind.scenario->resourceFor(taskId, personId, *ind.staff);
        TaskRef task = findTask(inst.root(), taskId);
        bool granted = task.item != nullptr && resource != nullptr && inst.mediator->requestResource(task.item, resource);
        trace.engine("ResourceCoordinator::requestResource(" + taskId + ")", "Mediator",
                     granted ? "re-obtained the slot held when the snapshot was taken" : "that slot is no longer free");
        if (!granted) {
            inst.data.erase("resourceBy." + taskId);
            inst.data.erase(it++);
        } else {
            ++it;
        }
    }

    inst.commands.reset(new CommandProcessor());
    inst.commandLabels.clear();
    trace.engine("new CommandProcessor()", "Command",
                 "undo history cleared: the old commands point at the tree that was just replaced");
    inst.started = inst.workflow->getStatus() != "Created";
    updateStatus(inst);
    inst.bus->publish(Event(EventType::EXTERNAL_EVENT, inst.id, "", "restored " + inst.snapshotLabels.back()));
    trace.engine("WorkflowEventBus::publish(EXTERNAL_EVENT, restored)", "Observer (Subject)", "");
    return respond(inst, true, "Restored " + inst.snapshotLabels.back() + ". Undo history cleared.", false, trace,
                   before, console.lines());
}

// ===========================================================================
// Views
// ===========================================================================

json EngineGateway::respond(Instance& inst, bool ok, const std::string& message, bool warning, Trace& trace,
                            const std::vector<Row>& before, const json& console) {
    std::vector<Row> after = rowsOf(inst.root());
    json changes = json::array();
    for (size_t i = 0; i < after.size() && i < before.size(); ++i) {
        if (after[i].state != before[i].state || after[i].assignee != before[i].assignee) {
            changes.push_back(json{{"task", after[i].id}, {"name", after[i].name},
                                   {"from", before[i].state}, {"to", after[i].state}});
        }
    }
    return json{{"ok", ok}, {"warning", warning}, {"message", message}, {"trace", traceJson(trace.steps())},
                {"console", console}, {"changes", changes}, {"instance", instanceView(inst)}};
}

json EngineGateway::treeNode(Industry& ind, const Data& data, WorkComponent* node, const std::vector<int>& path) {
    if (node->getState() == nullptr) {
        json children = json::array();
        for (int i = 0; node->getChild(i) != nullptr; ++i) {
            std::vector<int> childPath = path;
            childPath.push_back(i);
            children.push_back(treeNode(ind, data, node->getChild(i), childPath));
        }
        return json{{"type", "stage"}, {"name", ind.recorder->stageName(path)}, {"path", pathKey(path)},
                    {"children", children}};
    }

    TaskRef ref = refFor(node);
    const std::string id = ref.item->getId();
    json task = {{"type", "task"}, {"id", id}, {"name", ref.item->getName()}, {"kind", kindOf(ref)},
                 {"state", stateOf(node)}, {"canExecute", node->getState()->canExecute()},
                 {"decorators", ref.decorators}, {"priority", node->getPriority()}, {"path", pathKey(path)}};
    Participant* who = node->getAssignee();
    task["assignee"] = who ? json{{"id", who->getID()}, {"name", who->getName()}, {"role", who->getRole()}}
                           : json(nullptr);

    const TaskInfo* info = ind.info(id);
    if (info != nullptr) {
        task["roleHint"] = info->roleHint;
        task["help"] = info->help;
        task["outcomeQuestion"] = info->outcomeQuestion;
        task["outcomes"] = info->outcomes;
    }
    task["prerequisites"] = ind.prerequisites(id);

    if (ApprovalTask* approval = asApprovalTask(ref)) task["requestValue"] = approval->getRequestValue();
    if (AutomatedTask* automated = asAutomatedTask(ref)) task["stepSucceeds"] = automated->getStepSucceeds();

    Data::const_iterator held = data.find("resource." + id);
    task["resourceHeldWith"] = held == data.end() ? json(nullptr) : json(held->second);
    Data::const_iterator refused = data.find("refused." + id);
    task["resourceRefusedBy"] = refused == data.end() ? json(nullptr) : json(refused->second);
    return task;
}

json EngineGateway::instanceView(Instance& inst) {
    Industry& ind = *inst.industry;
    WorkComponent* root = inst.root();

    // "Available work" comes from the real ReadyTaskIterator.
    json available = json::array();
    ReadyTaskIterator ready(root);
    while (ready.hasNext()) {
        TaskRef ref = refFor(ready.next());
        if (ref.item != nullptr) available.push_back(ref.item->getId());
    }

    std::map<std::string, int> counts;
    std::vector<TaskRef> tasks = allTasks(root);
    for (size_t i = 0; i < tasks.size(); ++i) counts[stateOf(tasks[i].node)]++;

    json data = json::object();
    for (Data::const_iterator d = inst.data.begin(); d != inst.data.end(); ++d) data[d->first] = d->second;

    json commandHistory = json::array();
    for (size_t i = inst.commandLabels.size(); i > 0; --i) commandHistory.push_back(inst.commandLabels[i - 1]);

    return json{{"id", inst.id}, {"industry", ind.scenario->key()}, {"industryTitle", ind.scenario->title()},
                {"subject", json{{"id", inst.subjectId}, {"name", inst.subjectName}, {"summary", inst.summary},
                                 {"noun", ind.scenario->subjectNoun()}}},
                {"status", inst.workflow->getStatus()}, {"started", inst.started},
                {"outcome", data.value("outcome", "")}, {"data", data},
                {"tree", treeNode(ind, inst.data, root, std::vector<int>())},
                {"available", available}, {"counts", counts},
                {"audit", inst.audit->getLog()}, {"notifications", inst.notifier->getSent()},
                {"eventsTotal", inst.monitor->totalEvents()}, {"completedEvents", inst.monitor->completedFor(inst.id)},
                {"commandHistory", commandHistory},
                {"commandDepth", static_cast<int>(inst.commands->getHistory().size())},
                {"canUndo", !inst.commands->getHistory().empty()},
                {"snapshots", inst.snapshotLabels}};
}

json EngineGateway::summary(Instance& inst) {
    std::map<std::string, int> counts;
    std::vector<TaskRef> tasks = allTasks(inst.root());
    for (size_t i = 0; i < tasks.size(); ++i) counts[stateOf(tasks[i].node)]++;
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
    const std::vector<std::unique_ptr<StaffMember> >& people = ind.staff->members();
    for (size_t i = 0; i < people.size(); ++i) {
        Resource* slots = people[i]->slots.get();
        if (slots == nullptr) continue;
        json holders = json::array();
        for (size_t k = 0; k < instanceOrder.size(); ++k) {
            Instance& inst = *instances[instanceOrder[k]];
            if (inst.industry != &ind) continue;
            for (Data::const_iterator d = inst.data.begin(); d != inst.data.end(); ++d) {
                if (d->first.compare(0, 11, "resourceBy.") == 0 && d->second == people[i]->person->getID()) {
                    holders.push_back(json{{"instance", inst.id}, {"subject", inst.subjectName},
                                           {"task", d->first.substr(11)}});
                }
            }
        }
        out.push_back(json{{"id", people[i]->person->getID()}, {"name", people[i]->person->getName()},
                           {"resource", slots->getName()}, {"capacity", slots->getCapacity()},
                           {"available", slots->getAvailable()}, {"isFree", slots->isFree()},
                           {"grantedTo", holders}});
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
        const std::vector<std::unique_ptr<StaffMember> >& members = ind.staff->members();
        for (size_t k = 0; k < members.size(); ++k) {
            people.push_back(json{{"id", members[k]->person->getID()}, {"name", members[k]->person->getName()},
                                  {"role", members[k]->person->getRole()},
                                  {"slots", members[k]->slots ? members[k]->slots->getCapacity() : -1}});
        }
        json chain = json::array();
        std::vector<ChainLink> links = ind.scenario->chain();
        for (size_t k = 0; k < links.size(); ++k) {
            chain.push_back(json{{"role", links[k].role}, {"limit", links[k].limit}, {"final", links[k].isFinal}});
        }
        list.push_back(json{{"key", ind.scenario->key()}, {"title", ind.scenario->title()},
                            {"description", ind.scenario->description()}, {"noun", ind.scenario->subjectNoun()},
                            {"director", ind.scenario->directorName()}, {"definitionName", ind.definition->getName()},
                            {"decorated", ind.scenario->decoratedTasks()},
                            {"subjects", subjects}, {"participants", people}, {"chain", chain},
                            {"chainTask", ind.scenario->chainTaskId()},
                            {"definition", treeNode(ind, Data(), ind.definition->getRoot().get(), std::vector<int>())},
                            {"buildTrace", traceJson(ind.buildTrace)}});
    }
    return json{{"industries", list}, {"resetCount", resetCount},
                {"strategy", PendingIntegration::strategyStatus()}};
}
