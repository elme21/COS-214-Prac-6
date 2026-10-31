#include "EngineGateway.h"

#include "Trace.h"

// Generic scenario support (web/scenarios/)
#include "ScenarioSpec.h"
#include "ScenarioRuntime.h"
#include "Rules.h"
#include "StageNames.h"
#include "TaskLookup.h"

// The real TIVIDY engine (include/)
#include "WorkComponent.h"
#include "WorkItem.h"
#include "WorkItemState.h"
#include "AutomatedTask.h"
#include "ApprovalTask.h"
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
#include "TaskRouter.h"
#include "AssignmentStrategy.h"
#include "Event.h"
#include "WorkflowEventBus.h"
#include "DependencyActivator.h"
#include "AuditLogger.h"
#include "NotificationSender.h"
#include "MonitoringCollector.h"

#include <dirent.h>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>

typedef std::map<std::string, std::string> Data;
using nlohmann::json;

// ===========================================================================
// Internal records
// ===========================================================================

/** State of one task at a moment, to find what an action changed. */
struct EngineGateway::Row {
    std::string id;
    std::string name;
    std::string state;
    const Participant* assignee;
};

/** One running workflow instance: real engine objects only, plus display bookkeeping. */
struct EngineGateway::Instance {
    std::string id;
    std::string label;
    ScenarioRuntime* scenario;

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

    std::vector<std::shared_ptr<WorkComponent> > retiredRoots;            // replaced by a restore, kept alive
    std::vector<std::unique_ptr<ResourceCoordinator> > retiredMediators;

    std::map<std::string, Participant*> workloadCounted;  // task id -> participant whose workload counts it
    Data data;                                            // working copy; stored with WorkflowInstance::setData
    bool started;

    Instance() : scenario(nullptr), started(false) {}
    WorkComponent* root() const { return workflow->getRoot().get(); }
};

/** Everything one action needs to pass around. */
class EngineGateway::Context {
public:
    Context(Instance& instance, Trace& trace, const std::string& branch)
        : inst(instance), tr(trace), branchChoice(branch), warning(false) {}
    Instance& inst;
    Trace& tr;
    std::string branchChoice;   // a decision branch chosen in the UI (manual choice or override)
    std::vector<std::string> notices;
    bool warning;               // the action ran, but something the marker should notice was refused
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

std::string stateOf(const WorkComponent* c) {
    return c == nullptr || c->getState() == nullptr ? std::string("?") : c->getState()->name();
}

std::string operationName(const std::string& action) {
    if (action == "assign") return "assignTo(participant)";
    if (action == "autoAssign") return "TaskRouter::route()";
    return action + "()";
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

bool isNumber(const std::string& s) {
    if (s.empty()) return false;
    std::istringstream in(s);
    double v = 0;
    in >> v;
    return !in.fail() && in.eof();
}

std::string asText(const json& v) {
    if (v.is_string()) return v.get<std::string>();
    if (v.is_number_integer()) return std::to_string(v.get<long long>());
    if (v.is_number()) {
        std::ostringstream out;
        out << v.get<double>();
        return out.str();
    }
    if (v.is_boolean()) return v.get<bool>() ? "true" : "false";
    return "";
}

void facadeStep(Trace& trace, const std::string& operation) {
    trace.pending("WorkflowManager::" + operation, "Facade",
                  "not used: WorkflowManager keeps each instance's tree, mediator and command history private, "
                  "so a GUI cannot show or act on its tasks. EngineGateway does the same steps with the real classes");
}

/** The real command for each action. */
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
    static const char* names[] = { "makeAvailable", "assign", "autoAssign", "start", "execute", "complete",
                                   "approve", "reject", "cancel", "escalate" };
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i) if (action == names[i]) return true;
    return false;
}

bool readFile(const std::string& path, std::string& out) {
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in) return false;
    std::ostringstream buffer;
    buffer << in.rdbuf();
    out = buffer.str();
    return true;
}

} // namespace

// ===========================================================================
// Construction, templates and scenarios
// ===========================================================================

EngineGateway::EngineGateway(const std::string& dir) : templatesDir(dir), resetCount(0) {
    ConsoleCapture quiet;
    loadTemplates();
}

EngineGateway::~EngineGateway() {
    instances.clear();       // instances point into the scenarios and the registry, so they go first
    scenarioList.clear();
    registry.reset();
}

void EngineGateway::loadTemplates() {
    instances.clear();
    instanceOrder.clear();
    scenarioList.clear();
    templateFlags.clear();
    templateFiles.clear();
    templateErrors.clear();
    registry.reset(new DefinitionRegistry());

    std::vector<std::string> files;
    DIR* dir = opendir(templatesDir.c_str());
    if (dir == nullptr) {
        templateErrors.push_back("templates folder '" + templatesDir + "' not found");
        return;
    }
    while (dirent* entry = readdir(dir)) {
        std::string name = entry->d_name;
        if (name.size() > 5 && name.compare(name.size() - 5, 5, ".json") == 0) files.push_back(name);
    }
    closedir(dir);
    std::sort(files.begin(), files.end());

    for (size_t i = 0; i < files.size(); ++i) {
        std::string text;
        if (!readFile(templatesDir + "/" + files[i], text)) {
            templateErrors.push_back(files[i] + ": cannot be read");
            continue;
        }
        json spec = json::parse(text, nullptr, false);
        if (spec.is_discarded()) {
            templateErrors.push_back(files[i] + ": not valid JSON");
            continue;
        }
        std::vector<std::string> errors;
        if (addScenario(spec, true, files[i], errors) == nullptr) {
            for (size_t e = 0; e < errors.size(); ++e) templateErrors.push_back(files[i] + ": " + errors[e]);
        }
    }
    for (size_t e = 0; e < templateErrors.size(); ++e) std::cerr << "[templates] " << templateErrors[e] << std::endl;
}

ScenarioRuntime* EngineGateway::addScenario(const json& source, bool isTemplate, const std::string& file,
                                            std::vector<std::string>& errors) {
    ScenarioSpec spec;
    if (!parseScenarioSpec(source, spec, errors)) return nullptr;
    std::unique_ptr<ScenarioRuntime> runtime(new ScenarioRuntime(spec));
    for (size_t i = 0; i < scenarioList.size(); ++i) {
        if (scenarioList[i]->key() == runtime->key()) {
            errors.push_back("name: '" + spec.name + "' is already used by another scenario");
            return nullptr;
        }
    }
    std::string error;
    std::vector<std::string> steps;
    if (!runtime->build(*registry, error, steps)) {
        errors.push_back(error);
        return nullptr;
    }
    templateFlags[runtime->key()] = isTemplate;
    templateFiles[runtime->key()] = file;
    scenarioList.push_back(std::move(runtime));
    return scenarioList.back().get();
}

ScenarioRuntime& EngineGateway::scenarioOrThrow(const std::string& key) {
    for (size_t i = 0; i < scenarioList.size(); ++i) if (scenarioList[i]->key() == key) return *scenarioList[i];
    throw GatewayError(404, "Unknown scenario '" + key + "' (was the demo reset?)");
}

EngineGateway::Instance& EngineGateway::instanceOrThrow(const std::string& id) {
    std::map<std::string, std::unique_ptr<Instance> >::iterator it = instances.find(id);
    if (it == instances.end()) throw GatewayError(404, "No instance '" + id + "' (was the demo reset?)");
    return *it->second;
}

json EngineGateway::reset() {
    std::lock_guard<std::mutex> lock(mutex);
    ConsoleCapture console;
    loadTemplates();
    ++resetCount;
    return json{{"ok", true}, {"message", "Demo reset: custom scenarios removed, templates reloaded."},
                {"resetCount", resetCount}, {"templateErrors", templateErrors}};
}

json EngineGateway::templates() {
    std::lock_guard<std::mutex> lock(mutex);
    json list = json::array();
    for (size_t i = 0; i < scenarioList.size(); ++i) {
        ScenarioRuntime& s = *scenarioList[i];
        if (!templateFlags[s.key()]) continue;
        list.push_back(json{{"file", templateFiles[s.key()]}, {"name", s.spec().name},
                            {"industry", s.spec().industry}, {"description", s.spec().description},
                            {"spec", s.spec().source}});
    }
    return json{{"templates", list}, {"errors", templateErrors}};
}

json EngineGateway::validateSpec(const json& source) {
    std::lock_guard<std::mutex> lock(mutex);
    ScenarioSpec spec;
    std::vector<std::string> errors;
    bool ok = parseScenarioSpec(source, spec, errors);
    if (ok) {
        ScenarioRuntime probe(spec);
        for (size_t i = 0; i < scenarioList.size(); ++i) {
            if (scenarioList[i]->key() == probe.key()) errors.push_back("name: '" + spec.name + "' is already used by another scenario");
        }
        ok = errors.empty();
    }
    return json{{"ok", ok}, {"errors", errors},
                {"message", ok ? std::string("The spec is valid.") : std::to_string(errors.size()) + " problem(s) found."}};
}

json EngineGateway::createScenario(const json& source) {
    std::lock_guard<std::mutex> lock(mutex);
    ConsoleCapture console;
    std::vector<std::string> errors;
    ScenarioRuntime* created = addScenario(source, false, "", errors);
    if (created == nullptr) {
        throw GatewayError(400, "The scenario was not created: " + std::to_string(errors.size()) + " problem(s).", errors);
    }
    return json{{"ok", true}, {"message", "Scenario '" + created->spec().name + "' built with the real "
                                          "CodeDefinitionBuilder and registered in the DefinitionRegistry."},
                {"scenario", scenarioView(*created)}, {"console", console.lines()}};
}

json EngineGateway::scenarios() {
    std::lock_guard<std::mutex> lock(mutex);
    ConsoleCapture console;
    json list = json::array();
    for (size_t i = 0; i < scenarioList.size(); ++i) list.push_back(scenarioView(*scenarioList[i]));
    return json{{"scenarios", list}, {"templateErrors", templateErrors}};
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
    const std::vector<std::pair<std::string, std::string> >& deps = inst.scenario->definition()->getDependencies();
    for (size_t i = 0; i < deps.size(); ++i) {
        inst.mediator->addDependency(findTask(root, deps[i].second).item, findTask(root, deps[i].first).item);
    }
    std::ostringstream detail;
    detail << tasks.size() << " tasks registered; " << deps.size()
           << " dependencies from WorkflowDefinition::getDependencies() added for THIS instance's task objects";
    trace.engine("ResourceCoordinator::registerColleague / addDependency", "Mediator", detail.str());
}

void EngineGateway::configure(Instance& inst, Trace& trace) {
    const ScenarioSpec& spec = inst.scenario->spec();
    std::vector<const TaskSpec*> tasks = spec.allTasks();
    for (size_t i = 0; i < tasks.size(); ++i) {
        const TaskSpec& t = *tasks[i];
        TaskRef ref = findTask(inst.root(), t.id);
        if (ApprovalTask* approval = asApprovalTask(ref)) {
            double value = t.requestValue;
            std::string from = "fixed value";
            if (!t.requestField.empty()) {
                value = std::atof(inst.data[t.requestField].c_str());
                from = "input '" + t.requestField + "'";
            }
            approval->setRequestValue(value);
            std::ostringstream detail;
            detail << t.name << ": request value " << value << " (" << from << ") travels down the chain";
            trace.engine("ApprovalTask::setRequestValue", "Chain of Responsibility", detail.str());
        }
        if (AutomatedTask* automated = asAutomatedTask(ref)) {
            bool succeeds = t.successRule.present ? ruleHolds(t.successRule, inst.data) : t.succeeds;
            automated->setStepSucceeds(succeeds);
            if (!succeeds || t.successRule.present) {
                trace.scenario("AutomatedTask::setStepSucceeds(" + std::string(succeeds ? "true" : "false") + ")",
                               "scenario rule",
                               t.name + (t.successRule.present ? ": success rule " + describeRule(t.successRule) +
                                                                     (succeeds ? " holds" : " does NOT hold")
                                                               : ": fails by default"));
            }
        }
    }
}

void EngineGateway::publishChanges(Instance& inst, std::vector<Row>& seen, Trace& trace, bool fromUndo) {
    // Announce every task whose state changed, one at a time: publishing ITEM_COMPLETED lets the
    // real DependencyActivator unlock dependants, which are then announced in turn.
    for (int guard = 0; guard < 500; ++guard) {
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

void EngineGateway::syncWorkload(Instance& inst, Trace* trace) {
    // Participant::workload() feeds WorkloadAssignment, but no engine class updates it, so the
    // gateway calls the real itemAssigned()/itemReleased() whenever a task's holder changes.
    std::vector<TaskRef> tasks = allTasks(inst.root());
    std::vector<std::string> changes;
    for (size_t i = 0; i < tasks.size(); ++i) {
        std::string s = stateOf(tasks[i].node);
        Participant* holder = (s == "Assigned" || s == "InProgress") ? tasks[i].node->getAssignee() : nullptr;
        Participant*& counted = inst.workloadCounted[tasks[i].item->getId()];
        if (counted == holder) continue;
        if (counted != nullptr) { counted->itemReleased(); changes.push_back(counted->getName() + " -1"); }
        if (holder != nullptr) { holder->itemAssigned(); changes.push_back(holder->getName() + " +1"); }
        counted = holder;
    }
    if (trace != nullptr && !changes.empty()) {
        trace->temporary("Participant::itemAssigned / itemReleased", "Strategy (input)",
                         "workload " + join(changes, ", ") + " - called by EngineGateway because no engine class "
                         "updates it, and WorkloadAssignment reads it");
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

std::vector<EngineGateway::Row> EngineGateway::rowsOf(WorkComponent* root) {
    std::vector<Row> result;
    std::vector<TaskRef> tasks = allTasks(root);
    for (size_t i = 0; i < tasks.size(); ++i) {
        Row r = { tasks[i].item->getId(), tasks[i].item->getName(), stateOf(tasks[i].node), tasks[i].node->getAssignee() };
        result.push_back(r);
    }
    return result;
}

void EngineGateway::releaseResource(Instance& inst, const std::string& taskId, Trace& trace, const std::string& why) {
    TaskRef ref = findTask(inst.root(), taskId);
    if (ref.item == nullptr || inst.data.count("resourceId." + taskId) == 0) return;
    std::string name = inst.data["resource." + taskId];
    inst.mediator->release(ref.item);
    inst.data.erase("resource." + taskId);
    inst.data.erase("resourceId." + taskId);
    inst.data.erase("resourceBy." + taskId);
    trace.engine("ResourceCoordinator::release(" + ref.item->getName() + ")", "Mediator",
                 why + ": " + name + " gets its place back (Resource::giveBack)");
}

// ===========================================================================
// WorkflowManager mirror
// ===========================================================================

json EngineGateway::createInstance(const std::string& scenarioKey, const json& inputs, const std::string& label) {
    std::lock_guard<std::mutex> lock(mutex);
    ConsoleCapture console;
    ScenarioRuntime& scenario = scenarioOrThrow(scenarioKey);
    const ScenarioSpec& spec = scenario.spec();

    Data data;
    std::vector<std::string> problems;
    if (!inputs.is_null() && !inputs.is_object()) throw GatewayError(400, "inputs must be an object of field -> value");
    for (size_t i = 0; i < spec.inputs.size(); ++i) {
        const InputSpec& in = spec.inputs[i];
        std::string value;
        if (inputs.is_object()) {
            json::const_iterator it = inputs.find(in.key);
            if (it != inputs.end()) value = asText(*it);
        }
        if (in.type == "number" && !value.empty() && !isNumber(value)) problems.push_back(in.label + " must be a number");
        if (in.type == "choice" && !value.empty() &&
            std::find(in.options.begin(), in.options.end(), value) == in.options.end()) {
            problems.push_back(in.label + " must be one of: " + join(in.options, ", "));
        }
        data[in.key] = value;
    }
    if (inputs.is_object()) {
        for (json::const_iterator it = inputs.begin(); it != inputs.end(); ++it) {
            if (spec.input(it.key()) == nullptr) problems.push_back("'" + it.key() + "' is not an input field of this scenario");
        }
    }
    if (!problems.empty()) throw GatewayError(400, "The input data is not valid.", problems);

    Trace trace;
    facadeStep(trace, "createInstance(definitionName)");

    std::unique_ptr<Instance> inst(new Instance());
    inst->scenario = &scenario;
    std::ostringstream id;
    id << spec.idPrefix << "-" << scenario.nextInstanceNumber();
    inst->id = id.str();
    inst->data = data;
    inst->label = !label.empty() ? label : (!spec.subjectField.empty() && !data[spec.subjectField].empty()
                                                ? data[spec.subjectField] : inst->id);

    const WorkflowDefinition* def = registry->get(spec.name);
    trace.engine("DefinitionRegistry::get(\"" + def->getName() + "\")", "Prototype (PrototypeManager)", "");
    std::shared_ptr<WorkComponent> root = def->cloneRoot();
    std::ostringstream cloneDetail;
    cloneDetail << "deep copy: WorkStage::clone() copies each child (nested stages too), decorators clone what they "
                   "wrap; " << allTasks(root.get()).size() << " NEW task objects with the same IDs";
    trace.engine("WorkflowDefinition::cloneRoot()", "Prototype + Composite", cloneDetail.str());

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
    configure(*inst, trace);
    updateStatus(*inst);
    inst->bus->publish(Event(EventType::INSTANCE_CREATED, inst->id, "", "for " + inst->label));
    trace.engine("WorkflowEventBus::publish(INSTANCE_CREATED)", "Observer (Subject)", "");

    std::string newId = inst->id;
    instanceOrder.push_back(newId);
    instances[newId] = std::move(inst);
    Instance& created = *instances[newId];
    std::vector<Row> before = rowsOf(created.root());
    return respond(created, true, newId + " created for " + created.label + ". Press Start.", false, trace, before,
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

json EngineGateway::complete(const std::string& i, const std::string& t, const std::string& branch) {
    return act(i, t, "complete", "", branch);
}

json EngineGateway::getStatus(const std::string& instanceId) {
    std::lock_guard<std::mutex> lock(mutex);
    ConsoleCapture console;
    return instanceView(instanceOrThrow(instanceId));
}

json EngineGateway::makeAvailable(const std::string& i, const std::string& t) { return act(i, t, "makeAvailable", "", ""); }
json EngineGateway::assignTo(const std::string& i, const std::string& t, const std::string& p) { return act(i, t, "assign", p, ""); }
json EngineGateway::autoAssign(const std::string& i, const std::string& t) { return act(i, t, "autoAssign", "", ""); }
json EngineGateway::startTask(const std::string& i, const std::string& t) { return act(i, t, "start", "", ""); }
json EngineGateway::execute(const std::string& i, const std::string& t) { return act(i, t, "execute", "", ""); }
json EngineGateway::approve(const std::string& i, const std::string& t) { return act(i, t, "approve", "", ""); }
json EngineGateway::reject(const std::string& i, const std::string& t) { return act(i, t, "reject", "", ""); }
json EngineGateway::cancel(const std::string& i, const std::string& t) { return act(i, t, "cancel", "", ""); }
json EngineGateway::escalate(const std::string& i, const std::string& t) { return act(i, t, "escalate", "", ""); }

// ===========================================================================
// One lifecycle action
// ===========================================================================

json EngineGateway::act(const std::string& instanceId, const std::string& taskId, const std::string& action,
                        const std::string& participantId, const std::string& branch) {
    std::lock_guard<std::mutex> lock(mutex);
    ConsoleCapture console;
    Instance& inst = instanceOrThrow(instanceId);
    ScenarioRuntime& scenario = *inst.scenario;
    TaskRef task = findTask(inst.root(), taskId);
    if (task.node == nullptr) throw GatewayError(404, "No task '" + taskId + "' in " + instanceId);
    if (!isAction(action)) throw GatewayError(400, "Unknown action '" + action + "'");

    Trace trace;
    Context ctx(inst, trace, branch);
    facadeStep(trace, operationName(action));
    const std::string name = task.item->getName();
    const std::string before = stateOf(task.node);
    std::vector<Row> rowsBefore = rowsOf(inst.root());

    // --- checks that refuse without running a command -------------------------------
    std::string refusal;
    if (action == "makeAvailable") {
        bool can = inst.mediator->canBecomeAvailable(task.item);
        std::vector<std::string> waiting;
        const TaskSpec* spec = scenario.spec().task(taskId);
        for (size_t i = 0; spec != nullptr && i < spec->dependsOn.size(); ++i) {
            TaskRef p = findTask(inst.root(), spec->dependsOn[i]);
            if (p.item != nullptr && !p.item->isComplete()) waiting.push_back(p.item->getName() + " (" + stateOf(p.node) + ")");
        }
        trace.engine("ResourceCoordinator::canBecomeAvailable(" + name + ")", "Mediator",
                     can ? "true: every prerequisite is complete" : "false: waiting for " + join(waiting, ", "));
        if (!can) {
            refusal = "Dependency not met: \"" + name + "\" cannot become available until " + join(waiting, ", ") +
                      " is complete. The ResourceCoordinator (Mediator) refused.";
        }
    } else if (action == "assign") {
        if (scenario.participant(participantId) == nullptr) throw GatewayError(400, "Pick a participant to assign.");
    } else if (action == "autoAssign") {
        if (scenario.strategyFor(taskId) == nullptr) {
            refusal = "\"" + name + "\" has no assignment strategy in its scenario. Assign it manually.";
        }
    } else if (action == "start") {
        // Generic resource rule: a task that needs a resource only starts once the Mediator granted one.
        const TaskSpec* spec = scenario.spec().task(taskId);
        if (spec != nullptr && !spec->resource.empty() && inst.data.count("resourceId." + taskId) == 0) {
            refusal = "\"" + name + "\" needs a resource, and the ResourceCoordinator has not granted one for this "
                      "assignment. Escalate the task and reassign it to someone whose resource is free.";
            trace.scenario("resource rule (from the spec)", "Mediator", refusal);
        }
    }
    if (!refusal.empty()) return respond(inst, false, refusal, false, trace, rowsBefore, console.lines());

    bool ok = perform(inst, ctx, task, action, participantId);

    std::vector<Row> seen = rowsBefore;
    if (ok) publishChanges(inst, seen, trace, false);
    syncWorkload(inst, &trace);
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
    } else if (action == "autoAssign") {
        message = "Not assigned: the " + scenario.strategyName(taskId) + " strategy found no one, or \"" + name +
                  "\" is in state " + before + ", which does not allow assignment.";
    } else {
        message = "Not allowed: " + operationName(action) + " is not permitted while \"" + name + "\" is in state " +
                  before + ". The engine refused it (returned false); nothing changed.";
    }
    return respond(inst, ok, message, ctx.warning, trace, rowsBefore, consoleLines);
}

bool EngineGateway::perform(Instance& inst, Context& ctx, const TaskRef& task, const std::string& action,
                            const std::string& participantId) {
    ScenarioRuntime& scenario = *inst.scenario;
    Trace& trace = ctx.tr;
    const std::string id = task.item->getId();
    const std::string name = task.item->getName();
    const std::string from = stateOf(task.node);

    bool ok = false;
    std::string assignee = participantId;
    if (action == "execute") {
        ok = runTemplateMethod(inst, ctx, task);
    } else if (action == "autoAssign") {
        AssignmentStrategy* strategy = scenario.strategyFor(id);
        TaskRouter router(strategy);
        ok = router.route(task.item);
        Participant* chosen = task.node->getAssignee();
        trace.engine("TaskRouter::route(" + name + ") -> " + scenario.strategyName(id) + "::route()", "Strategy",
                     ok ? "chose " + chosen->getName() + " (" + chosen->getRole() + ") and called WorkItem::assignTo()"
                        : "returned false: nobody suitable, or the task's state refused assignTo()");
        if (ok) {
            trace.engine(name + ".assignTo(" + chosen->getName() + ")", "State",
                         "WorkItem delegates to " + from + "State -> AssignedState. Not undoable: the strategy assigns "
                         "the task directly, not through AssignCommand");
            assignee = chosen->getID();
        }
    } else {
        Participant* who = action == "assign" ? scenario.participant(participantId) : nullptr;
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

    if (action == "assign" || action == "autoAssign") afterAssign(inst, ctx, task, assignee);
    const std::string to = stateOf(task.node);
    if (from != to) afterTransition(inst, ctx, task, to);
    return true;
}

void EngineGateway::afterAssign(Instance& inst, Context& ctx, const TaskRef& task, const std::string& participantId) {
    ScenarioRuntime& scenario = *inst.scenario;
    const std::string id = task.item->getId();
    const std::string name = task.item->getName();
    Participant* who = scenario.participant(participantId);
    if (who == nullptr) return;
    ctx.tr.engine("Participant " + who->getName(), "Mediator (colleague)", who->getRole() + " is now the assignee");

    const TaskSpec* spec = scenario.spec().task(id);
    if (spec == nullptr || spec->resource.empty()) return;
    Resource* resource = scenario.resourceFor(id, participantId);
    if (inst.data.count("resourceId." + id)) releaseResource(inst, id, ctx.tr, "reassigned");
    if (resource == nullptr) {
        inst.data["refused." + id] = who->getName();
        ctx.warning = true;
        ctx.notices.push_back(who->getName() + " has no resource of their own for this task. Escalate, then reassign.");
        return;
    }
    bool granted = inst.mediator->requestResource(task.item, resource);
    std::ostringstream places;
    places << resource->getAvailable() << " of " << resource->getCapacity() << " places now free";
    ctx.tr.engine("ResourceCoordinator::requestResource(" + name + ", " + resource->getName() + ")", "Mediator",
                  granted ? "GRANTED: Resource::take() - " + places.str()
                          : "REFUSED: Resource::isFree() is false - " + places.str());
    if (granted) {
        inst.data["resource." + id] = resource->getName();
        inst.data["resourceId." + id] = scenario.resourceIdOf(resource);
        inst.data["resourceBy." + id] = who->getID();
        inst.data.erase("refused." + id);
        ctx.notices.push_back("Resource granted: " + resource->getName() + " (" + places.str() + ").");
    } else {
        inst.data["refused." + id] = resource->getName();
        ctx.warning = true;
        ctx.notices.push_back("BUT the ResourceCoordinator refused " + resource->getName() +
                              " (no free place). Escalate this task, then reassign it.");
    }
}

void EngineGateway::afterTransition(Instance& inst, Context& ctx, const TaskRef& task, const std::string& newState) {
    const std::string id = task.item->getId();
    if (newState == "Completed") {
        if (inst.scenario->spec().decisionAfter(id) != nullptr) decide(inst, ctx, id);
        applyEffects(inst, ctx, id, "onComplete");
    } else if (newState == "Rejected") {
        applyEffects(inst, ctx, id, "onReject");
    } else if (newState == "Cancelled") {
        releaseResource(inst, id, ctx.tr, "the holder was cancelled");
        applyEffects(inst, ctx, id, "onCancel");
    }
}

void EngineGateway::decide(Instance& inst, Context& ctx, const std::string& taskId) {
    const DecisionSpec& d = *inst.scenario->spec().decisionAfter(taskId);
    std::string reason;
    std::string ruled = d.rules.empty() ? std::string() : branchByRules(d, inst.data, reason);
    std::string chosen;
    std::string how;
    bool valid = false;
    for (size_t b = 0; b < d.branches.size(); ++b) if (d.branches[b].id == ctx.branchChoice) valid = true;
    if (valid) {
        chosen = ctx.branchChoice;
        how = d.mode == "rule" ? (chosen == ruled ? "rule result confirmed by the user" : "MANUAL OVERRIDE of the rule (" + reason + ")")
                               : "chosen by the user at runtime";
    } else if (d.mode == "rule") {
        chosen = ruled;
        how = "decided by the rule: " + reason;
    } else {
        chosen = !ruled.empty() ? ruled : d.defaultBranch;
        how = "no branch chosen in the UI: default branch";
    }

    const BranchSpec* taken = nullptr;
    for (size_t b = 0; b < d.branches.size(); ++b) if (d.branches[b].id == chosen) taken = &d.branches[b];
    if (taken == nullptr) taken = &d.branches[0];
    inst.data[d.id] = taken->id;
    ctx.tr.scenario("Decision '" + d.label + "' (from the spec)", "routing (decision)",
                    "branch '" + taken->label + "' - " + how + "; the other branches are cancelled");
    ctx.notices.push_back("Decision \"" + d.label + "\": " + taken->label + " (" + how + ").");
    for (size_t b = 0; b < d.branches.size(); ++b) {
        if (&d.branches[b] == taken) continue;
        for (size_t t = 0; t < d.branches[b].tasks.size(); ++t) {
            TaskRef ref = findTask(inst.root(), d.branches[b].tasks[t]);
            if (ref.node == nullptr) continue;
            std::string before = stateOf(ref.node);
            bool ok = runCommand(inst, new CancelCommand(ref.node), "CancelCommand(" + ref.item->getId() + ")");
            ctx.tr.engine("CancelCommand(" + ref.item->getName() + ") -> CommandProcessor::run", "Command + State",
                          ok ? before + "State -> CancelledState (branch '" + d.branches[b].label + "' not taken)"
                             : before + "State refused cancel()");
        }
    }
}

void EngineGateway::applyEffects(Instance& inst, Context& ctx, const std::string& taskId, const std::string& when) {
    const TaskSpec* spec = inst.scenario->spec().task(taskId);
    if (spec == nullptr) return;
    const EffectSpec& e = when == "onComplete" ? spec->onComplete : when == "onReject" ? spec->onReject : spec->onCancel;
    if (e.empty()) return;
    ctx.tr.scenario(spec->name + " " + when + " (from the spec)", "scenario rule",
                    "effects run with real engine calls; they do not cascade");
    for (CaseData::const_iterator it = e.set.begin(); it != e.set.end(); ++it) inst.data[it->first] = fillTemplate(it->second, inst.data);
    if (!e.outcome.empty()) inst.data["outcome"] = fillTemplate(e.outcome, inst.data);
    for (size_t i = 0; i < e.release.size(); ++i) releaseResource(inst, e.release[i], ctx.tr, spec->name + " " + when);
    for (size_t i = 0; i < e.cancel.size(); ++i) {
        TaskRef ref = findTask(inst.root(), e.cancel[i]);
        if (ref.node == nullptr) continue;
        std::string before = stateOf(ref.node);
        bool ok = runCommand(inst, new CancelCommand(ref.node), "CancelCommand(" + ref.item->getId() + ")");
        ctx.tr.engine("CancelCommand(" + ref.item->getName() + ") -> CommandProcessor::run", "Command + State",
                      ok ? before + "State -> CancelledState" : before + "State refused cancel(); left as " + before);
        if (ok) releaseResource(inst, e.cancel[i], ctx.tr, "the holder was cancelled");
    }
    for (size_t i = 0; i < e.unlock.size(); ++i) {
        TaskRef ref = findTask(inst.root(), e.unlock[i]);
        if (ref.node == nullptr) continue;
        std::string before = stateOf(ref.node);
        bool ok = runCommand(inst, new MakeAvailableCommand(ref.node), "MakeAvailableCommand(" + ref.item->getId() + ")");
        ctx.tr.engine("MakeAvailableCommand(" + ref.item->getName() + ") -> CommandProcessor::run", "Command + State",
                      ok ? before + "State -> AvailableState" : before + "State refused makeAvailable()");
    }
}

bool EngineGateway::runTemplateMethod(Instance& inst, Context& ctx, const TaskRef& task) {
    Trace& trace = ctx.tr;
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

    task.node->execute();   // Template Method: checkReady -> onEnter -> perform -> recordResult
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
        // The chain's result as ApprovalTask kept it (path, outcome, decider): no web-side tracing needed.
        const ApprovalTask& approval = *asApprovalTask(task);
        std::vector<ChainLevelInfo> levels = inst.scenario->chainOf(task.item->getId());
        trace.engine(name + ".execute()", "Template Method",
                     "checkReady() true -> onEnter() -> ApprovalTask::perform() sends an ApprovalRequest(value) down "
                     "the chain (ApprovalHandler::handleRequest -> tryDecide at each level) and keeps the result");
        const std::vector<std::string>& path = approval.getChainPath();
        std::ostringstream value;
        value << approval.getRequestValue();
        for (size_t i = 0; i < path.size(); ++i) {
            bool last = i + 1 == path.size();
            trace.engine("ApprovalHandler [" + path[i] + "]", "Chain of Responsibility",
                         "request value " + value.str() + (last ? " stopped here" : ": over this level's limit, passed on"));
        }
        if (approval.wasChainDecided()) {
            const std::string& decider = approval.getChainDecidedBy();
            std::string handlerClass = !levels.empty() && decider == levels.back().label ? "FinalApproverHandler"
                                                                                       : "RoleApproverHandler";
            trace.engine(handlerClass + " [" + decider + "]::tryDecide", "Chain of Responsibility",
                         approval.wasChainApproved() ? "within its limit: approves" : "over the final limit: rejects");
            ctx.notices.push_back("The request travelled " + join(path, " -> ") + "; " + decider + " " +
                                  approval.getChainOutcome() + " it.");
        } else {
            ctx.notices.push_back("Chain outcome: " + approval.getChainOutcome() + ".");
        }
        trace.engine("ApprovalTask::getChainPath / getChainOutcome / getChainDecidedBy", "Chain of Responsibility",
                     "outcome '" + approval.getChainOutcome() + "' read from the result ApprovalTask kept from the "
                     "ApprovalRequest");
        if (to == "Rejected") ctx.warning = true;
        trace.engine(name + ".approve() / reject()", "State",
                     "called by the deciding handler on the task itself (bypassing its decorators): " + from +
                         "State -> " + to + "State");
    }

    if (from != to) afterTransition(inst, ctx, task, to);
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
                     "effects on other tasks or resources are not undone by the engine's commands");
    }
    std::vector<Row> seen = before;
    publishChanges(inst, seen, trace, true);
    syncWorkload(inst, &trace);
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
    ScenarioRuntime& scenario = *inst.scenario;
    Trace trace;
    std::vector<Row> before = rowsOf(inst.root());

    std::shared_ptr<InstanceSnapshot> snap = inst.snapshots.latest();
    if (!snap) return respond(inst, false, "Take a snapshot first.", false, trace, before, console.lines());
    trace.engine("SnapshotHistory::latest()", "Memento (Caretaker)", "hands back the newest snapshot");

    // Resources are held in the Mediator, outside the snapshot: hand back the old tree's places.
    std::vector<TaskRef> oldTasks = allTasks(inst.root());
    for (size_t i = 0; i < oldTasks.size(); ++i) inst.mediator->release(oldTasks[i].item);
    trace.engine("ResourceCoordinator::release(every old task)", "Mediator", "the replaced tree gives its resources back");

    inst.retiredRoots.push_back(inst.workflow->getRoot());
    inst.retiredMediators.push_back(std::move(inst.mediator));
    inst.workflow->restoreSnapshot(snap);
    trace.engine("WorkflowInstance::restoreSnapshot(snapshot)", "Memento (Originator)",
                 "restores status '" + inst.workflow->getStatus() + "' and a fresh clone of the saved tree");
    inst.mediator.reset(new ResourceCoordinator());
    wire(inst, trace);

    inst.data = inst.snapshotData.back();
    std::vector<std::string> keys;
    for (Data::const_iterator it = inst.data.begin(); it != inst.data.end(); ++it) {
        if (it->first.compare(0, 11, "resourceId.") == 0) keys.push_back(it->first.substr(11));
    }
    for (size_t k = 0; k < keys.size(); ++k) {
        const std::string& taskId = keys[k];
        Resource* resource = scenario.resource(inst.data["resourceId." + taskId]);
        TaskRef task = findTask(inst.root(), taskId);
        bool granted = task.item != nullptr && resource != nullptr && inst.mediator->requestResource(task.item, resource);
        trace.engine("ResourceCoordinator::requestResource(" + taskId + ")", "Mediator",
                     granted ? "re-obtained the resource held when the snapshot was taken" : "that resource is no longer free");
        if (!granted) {
            inst.data.erase("resource." + taskId);
            inst.data.erase("resourceId." + taskId);
            inst.data.erase("resourceBy." + taskId);
        }
    }

    inst.commands.reset(new CommandProcessor());
    inst.commandLabels.clear();
    trace.engine("new CommandProcessor()", "Command",
                 "undo history cleared: the old commands point at the tree that was just replaced");
    inst.started = inst.workflow->getStatus() != "Created";
    syncWorkload(inst, &trace);
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

json EngineGateway::treeNode(ScenarioRuntime& scenario, const Data& data, WorkComponent* node, const std::vector<int>& path) {
    if (node->getState() == nullptr) {
        json children = json::array();
        for (int i = 0; node->getChild(i) != nullptr; ++i) {
            std::vector<int> childPath = path;
            childPath.push_back(i);
            children.push_back(treeNode(scenario, data, node->getChild(i), childPath));
        }
        return json{{"type", "stage"}, {"name", scenario.stageNames().nameAt(path)}, {"path", pathKey(path)},
                    {"children", children}};
    }

    TaskRef ref = refFor(node);
    const std::string id = ref.item->getId();
    const ScenarioSpec& spec = scenario.spec();
    const TaskSpec* t = spec.task(id);
    json task = {{"type", "task"}, {"id", id}, {"name", ref.item->getName()}, {"kind", kindOf(ref)},
                 {"state", stateOf(node)}, {"canExecute", node->getState()->canExecute()},
                 {"decorators", ref.decorators}, {"path", pathKey(path)}};
    Participant* who = node->getAssignee();
    task["assignee"] = who ? json{{"id", who->getID()}, {"name", who->getName()}, {"role", who->getRole()}} : json(nullptr);
    if (t != nullptr) {
        task["role"] = t->role;
        task["help"] = t->help;
        task["prerequisites"] = t->dependsOn;
        task["strategy"] = scenario.strategyName(id);
        task["resource"] = t->resource == "@assignee" ? std::string("the assignee's own resource")
                                                      : (t->resource.empty() ? std::string()
                                                                             : spec.resource(t->resource)->name);
    }
    if (ApprovalTask* approval = asApprovalTask(ref)) {
        task["requestValue"] = approval->getRequestValue();
        json chain = json::array();
        std::vector<ChainLevelInfo> levels = scenario.chainOf(id);
        for (size_t i = 0; i < levels.size(); ++i) {
            chain.push_back(json{{"label", levels[i].label}, {"role", levels[i].role}, {"limit", levels[i].limit},
                                 {"final", levels[i].isFinal}});
        }
        task["chain"] = chain;
        task["chainResult"] = json{{"ran", approval->hasChainResult()},
                                   {"outcome", approval->getChainOutcome()},
                                   {"decided", approval->wasChainDecided()},
                                   {"approved", approval->wasChainApproved()},
                                   {"path", approval->getChainPath()},
                                   {"decidedBy", approval->getChainDecidedBy()}};
    }
    if (AutomatedTask* automated = asAutomatedTask(ref)) task["stepSucceeds"] = automated->getStepSucceeds();

    if (const DecisionSpec* d = spec.decisionAfter(id)) {
        json branches = json::array();
        for (size_t b = 0; b < d->branches.size(); ++b) {
            branches.push_back(json{{"id", d->branches[b].id}, {"label", d->branches[b].label}});
        }
        std::string reason;
        std::string suggested = d->rules.empty() ? d->defaultBranch : branchByRules(*d, data, reason);
        task["decision"] = json{{"id", d->id}, {"label", d->label}, {"mode", d->mode}, {"branches", branches},
                                {"suggested", suggested}, {"reason", reason}};
    }
    for (size_t i = 0; i < spec.decisions.size(); ++i) {
        for (size_t b = 0; b < spec.decisions[i].branches.size(); ++b) {
            const std::vector<std::string>& ts = spec.decisions[i].branches[b].tasks;
            if (std::find(ts.begin(), ts.end(), id) != ts.end()) {
                task["branchOf"] = spec.decisions[i].label + ": " + spec.decisions[i].branches[b].label;
            }
        }
    }

    Data::const_iterator held = data.find("resource." + id);
    task["resourceHeld"] = held == data.end() ? json(nullptr) : json(held->second);
    Data::const_iterator refused = data.find("refused." + id);
    task["resourceRefused"] = refused == data.end() ? json(nullptr) : json(refused->second);
    return task;
}

json EngineGateway::instanceView(Instance& inst) {
    ScenarioRuntime& scenario = *inst.scenario;
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

    return json{{"id", inst.id}, {"scenario", scenario.key()}, {"scenarioName", scenario.spec().name},
                {"label", inst.label}, {"status", inst.workflow->getStatus()}, {"started", inst.started},
                {"outcome", data.value("outcome", "")}, {"data", data},
                {"tree", treeNode(scenario, inst.data, root, std::vector<int>())},
                {"available", available}, {"counts", counts},
                {"audit", inst.audit->getLog()}, {"notifications", inst.notifier->getSent()},
                {"eventsTotal", inst.monitor->totalEvents()},
                {"commandHistory", commandHistory},
                {"commandDepth", static_cast<int>(inst.commands->getHistory().size())},
                {"canUndo", !inst.commands->getHistory().empty()},
                {"snapshots", inst.snapshotLabels}};
}

json EngineGateway::scenarioView(ScenarioRuntime& scenario) {
    const ScenarioSpec& spec = scenario.spec();
    json people = json::array();
    for (size_t i = 0; i < scenario.participants().size(); ++i) {
        Participant* p = scenario.participants()[i].get();
        people.push_back(json{{"id", p->getID()}, {"name", p->getName()}, {"role", p->getRole()},
                              {"workload", p->workload()}});
    }
    json resourceList = json::array();
    for (size_t i = 0; i < spec.resources.size(); ++i) {
        Resource* r = scenario.resource(spec.resources[i].id);
        const ParticipantSpec* owner = spec.participant(spec.resources[i].owner);
        resourceList.push_back(json{{"id", spec.resources[i].id}, {"name", r->getName()}, {"capacity", r->getCapacity()},
                                    {"available", r->getAvailable()}, {"owner", owner ? owner->name : std::string()}});
    }
    json inputs = json::array();
    for (size_t i = 0; i < spec.inputs.size(); ++i) {
        inputs.push_back(json{{"key", spec.inputs[i].key}, {"label", spec.inputs[i].label},
                              {"type", spec.inputs[i].type}, {"options", spec.inputs[i].options}});
    }
    json seeds = json::array();
    for (size_t i = 0; i < spec.seeds.size(); ++i) {
        json data = json::object();
        for (CaseData::const_iterator it = spec.seeds[i].data.begin(); it != spec.seeds[i].data.end(); ++it) data[it->first] = it->second;
        seeds.push_back(json{{"label", spec.seeds[i].label}, {"summary", spec.seeds[i].summary}, {"data", data}});
    }
    json chains = json::array();
    std::vector<const TaskSpec*> tasks = spec.allTasks();
    for (size_t i = 0; i < tasks.size(); ++i) {
        std::vector<ChainLevelInfo> levels = scenario.chainOf(tasks[i]->id);
        if (levels.empty()) continue;
        json lv = json::array();
        for (size_t k = 0; k < levels.size(); ++k) {
            lv.push_back(json{{"label", levels[k].label}, {"role", levels[k].role}, {"limit", levels[k].limit},
                              {"final", levels[k].isFinal}});
        }
        chains.push_back(json{{"task", tasks[i]->id}, {"taskName", tasks[i]->name}, {"levels", lv}});
    }
    json decisions = json::array();
    for (size_t i = 0; i < spec.decisions.size(); ++i) {
        const DecisionSpec& d = spec.decisions[i];
        json branches = json::array();
        for (size_t b = 0; b < d.branches.size(); ++b) branches.push_back(json{{"id", d.branches[b].id}, {"label", d.branches[b].label}});
        json rules = json::array();
        for (size_t r = 0; r < d.rules.size(); ++r) rules.push_back(describeRule(d.rules[r]) + " -> " + d.rules[r].branch);
        const TaskSpec* after = spec.task(d.after);
        decisions.push_back(json{{"id", d.id}, {"label", d.label}, {"after", after ? after->name : d.after},
                                 {"mode", d.mode}, {"branches", branches}, {"rules", rules}});
    }
    return json{{"key", scenario.key()}, {"name", spec.name}, {"industry", spec.industry},
                {"description", spec.description}, {"idPrefix", spec.idPrefix}, {"subjectField", spec.subjectField},
                {"template", templateFlags[scenario.key()]}, {"templateFile", templateFiles[scenario.key()]},
                {"inputs", inputs}, {"participants", people}, {"resources", resourceList}, {"seeds", seeds},
                {"chains", chains}, {"decisions", decisions},
                {"definition", treeNode(scenario, Data(), scenario.definition()->getRoot().get(), std::vector<int>())},
                {"spec", spec.source}};
}

json EngineGateway::summary(Instance& inst) {
    std::map<std::string, int> counts;
    std::vector<TaskRef> tasks = allTasks(inst.root());
    for (size_t i = 0; i < tasks.size(); ++i) counts[stateOf(tasks[i].node)]++;
    return json{{"id", inst.id}, {"scenario", inst.scenario->key()}, {"label", inst.label},
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

json EngineGateway::resources(const std::string& scenarioKey) {
    std::lock_guard<std::mutex> lock(mutex);
    ConsoleCapture console;
    ScenarioRuntime& scenario = scenarioOrThrow(scenarioKey);
    const ScenarioSpec& spec = scenario.spec();
    json out = json::array();
    for (size_t i = 0; i < spec.resources.size(); ++i) {
        Resource* r = scenario.resource(spec.resources[i].id);
        json holders = json::array();
        for (size_t k = 0; k < instanceOrder.size(); ++k) {
            Instance& inst = *instances[instanceOrder[k]];
            if (inst.scenario != &scenario) continue;
            for (Data::const_iterator d = inst.data.begin(); d != inst.data.end(); ++d) {
                if (d->first.compare(0, 11, "resourceId.") == 0 && d->second == spec.resources[i].id) {
                    holders.push_back(json{{"instance", inst.id}, {"label", inst.label}, {"task", d->first.substr(11)}});
                }
            }
        }
        const ParticipantSpec* owner = spec.participant(spec.resources[i].owner);
        out.push_back(json{{"id", spec.resources[i].id}, {"name", r->getName()}, {"capacity", r->getCapacity()},
                           {"available", r->getAvailable()}, {"isFree", r->isFree()},
                           {"owner", owner ? owner->name : std::string()}, {"heldBy", holders}});
    }
    return out;
}

json EngineGateway::meta() {
    std::lock_guard<std::mutex> lock(mutex);
    json list = json::array();
    for (size_t i = 0; i < scenarioList.size(); ++i) {
        list.push_back(json{{"key", scenarioList[i]->key()}, {"name", scenarioList[i]->spec().name},
                            {"industry", scenarioList[i]->spec().industry}, {"template", templateFlags[scenarioList[i]->key()]}});
    }
    return json{{"scenarios", list}, {"resetCount", resetCount}, {"templateErrors", templateErrors}};
}
