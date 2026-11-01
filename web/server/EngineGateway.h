#ifndef WEB_ENGINEGATEWAY_H
#define WEB_ENGINEGATEWAY_H

#include "json.hpp"

#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

class Trace;
class WorkComponent;
class WorkItem;
class DefinitionRegistry;
class TaskRouter;
class WorkflowManager;
class ScenarioRuntime;

/// @brief An error the HTTP layer turns into a status code and a friendly message (plus optional details).
class GatewayError : public std::runtime_error {
public:
    /// @brief Creates the error.
    /// @param httpStatus the HTTP status to answer with (e.g. 400, 404)
    /// @param message the friendly message shown to the user
    /// @param details optional list of individual problems (e.g. spec validation errors)
    GatewayError(int httpStatus, const std::string& message,
                 const std::vector<std::string>& details = std::vector<std::string>())
        : std::runtime_error(message), status(httpStatus), errors(details) {}
    int status;                       ///< HTTP status to answer with
    std::vector<std::string> errors;  ///< Individual problems, may be empty
};

/// @brief One task of a case, as WorkflowManager hands it out.
struct CaseTask {
    std::string id;        ///< Task id
    WorkComponent* node;   ///< Outermost component (WorkflowManager::getTask); decorators included, commands act on it
    WorkItem* item;        ///< The WorkItem inside all decorators (WorkflowManager::getItem)
};

/**
 * @brief The ONLY place the web server touches the TIVIDY engine: a thin JSON <-> WorkflowManager layer.
 *
 * Every scenario is a ScenarioSpec (a .json file in web/templates or made in the wizard), built by the
 * GenericDirector on the real CodeDefinitionBuilder and registered in the real DefinitionRegistry.
 * Nothing here knows about any industry.
 *
 * Every case is created, started, inspected and acted on through the Facade, WorkflowManager:
 * createInstance, start, getStatus, getTree / getTask / getItem / getTaskIds, makeAvailable, assign,
 * start(task), complete, approve, reject, cancel, escalate, undo and restoreSnapshot. Where the Facade
 * has no operation, the gateway makes the smallest direct call on an object the Facade hands out, and
 * labels it in the trace: execute() (Template Method), auto-assign of one task (TaskRouter), creating a
 * snapshot (WorkflowInstance), resource requests (the case's ResourceCoordinator), and events
 * (WorkflowManager publishes none, so each case has its own WorkflowEventBus here).
 *
 * Thread safety: one mutex around every public method (httplib uses a thread pool).
 */
class EngineGateway {
public:
    /// @brief The JSON type used for every request and reply.
    typedef nlohmann::json Json;

    /// @brief Loads every template spec and builds it.
    /// @param templatesDir folder holding the template .json files
    explicit EngineGateway(const std::string& templatesDir);
    /// @brief Deletes the cases, then the Facade, the scenarios and the registry they use.
    ~EngineGateway();

    /// @brief Discards every case and custom scenario, then reloads the templates (with a new WorkflowManager).
    /// @return {ok, message, resetCount, templateErrors}
    Json reset();
    /// @brief Lists the template specs (for the wizard's "Use a template").
    /// @return {templates, errors}
    Json templates();
    /// @brief Describes every scenario: inputs, people, resources, chains, decisions, seeds, tree and spec.
    /// @return {scenarios, templateErrors}
    Json scenarios();
    /// @brief Checks a spec without building it.
    /// @param spec the spec as JSON
    /// @return {ok, errors, message}
    Json validateSpec(const Json& spec);
    /// @brief Validates a spec, builds it with the GenericDirector and registers it.
    /// @param spec the spec as JSON
    /// @return {ok, message, scenario, console}; throws GatewayError(400) with the problems if invalid
    Json createScenario(const Json& spec);
    /// @brief Summarises every case (status, outcome, state counts).
    /// @return an array of summaries
    Json listInstances();
    /// @brief Lists a scenario's resources with capacity, free places and holders.
    /// @param scenarioKey the scenario's key
    /// @return an array of resources
    Json resources(const std::string& scenarioKey);

    /// @brief Creates a case with WorkflowManager::createInstance and gives it its event bus.
    /// @param scenarioKey the scenario's key
    /// @param inputs input field -> value for the new case
    /// @param label optional title; the subject field's value is used if empty
    /// @return the action reply with the new case view
    Json createInstance(const std::string& scenarioKey, const Json& inputs, const std::string& label);
    /// @brief Starts a case with WorkflowManager::start.
    /// @param instanceId the case id
    /// @return the action reply
    Json start(const std::string& instanceId);
    /// @brief Completes a task with WorkflowManager::complete, then runs the decision and effects.
    /// @param instanceId the case id
    /// @param taskId the task id
    /// @param branch for a task followed by a decision: the branch chosen in the UI (may be empty)
    /// @return the action reply
    Json complete(const std::string& instanceId, const std::string& taskId, const std::string& branch);
    /// @brief The full case view (WorkflowManager::getStatus and the tree from getTree).
    /// @param instanceId the case id
    /// @return the case view
    Json getStatus(const std::string& instanceId);

    /// @brief Calls WorkflowManager::makeAvailable.
    /// @param instanceId the case id
    /// @param taskId the task id
    /// @return the action reply
    Json makeAvailable(const std::string& instanceId, const std::string& taskId);
    /// @brief Calls WorkflowManager::assign, then asks the case's Mediator for the task's resource if it needs one.
    /// @param instanceId the case id
    /// @param taskId the task id
    /// @param participantId the participant to assign
    /// @return the action reply
    Json assignTo(const std::string& instanceId, const std::string& taskId, const std::string& participantId);
    /// @brief Routes one task with the TaskRouter (WorkflowManager has no per-task auto-assign).
    /// @param instanceId the case id
    /// @param taskId the task id
    /// @return the action reply
    Json autoAssign(const std::string& instanceId, const std::string& taskId);
    /// @brief Calls WorkflowManager::start(case, task) (refused first if the task needs a resource it does not hold).
    /// @param instanceId the case id
    /// @param taskId the task id
    /// @return the action reply
    Json startTask(const std::string& instanceId, const std::string& taskId);
    /// @brief Calls execute() (Template Method) on the task from WorkflowManager::getTask (the Facade has no execute).
    /// @param instanceId the case id
    /// @param taskId the task id
    /// @return the action reply
    Json execute(const std::string& instanceId, const std::string& taskId);
    /// @brief Calls WorkflowManager::approve.
    /// @param instanceId the case id
    /// @param taskId the task id
    /// @return the action reply
    Json approve(const std::string& instanceId, const std::string& taskId);
    /// @brief Calls WorkflowManager::reject.
    /// @param instanceId the case id
    /// @param taskId the task id
    /// @return the action reply
    Json reject(const std::string& instanceId, const std::string& taskId);
    /// @brief Calls WorkflowManager::cancel.
    /// @param instanceId the case id
    /// @param taskId the task id
    /// @return the action reply
    Json cancel(const std::string& instanceId, const std::string& taskId);
    /// @brief Calls WorkflowManager::escalate.
    /// @param instanceId the case id
    /// @param taskId the task id
    /// @return the action reply
    Json escalate(const std::string& instanceId, const std::string& taskId);

    /// @brief Calls WorkflowManager::undo.
    /// @param instanceId the case id
    /// @return the action reply
    Json undo(const std::string& instanceId);
    /// @brief Saves a snapshot with WorkflowInstance::createSnapshot() (instance from WorkflowManager::getInstance).
    /// @param instanceId the case id
    /// @return the action reply
    Json snapshot(const std::string& instanceId);
    /// @brief Restores the newest snapshot with WorkflowManager::restoreSnapshot.
    /// @param instanceId the case id
    /// @return the action reply
    Json restoreSnapshot(const std::string& instanceId);

    /// @brief The state of one task at a moment, used to find what an action changed.
    struct Row;

private:
    /// @brief One case: its WorkflowManager id plus what the Facade does not keep (events, snapshots, data).
    struct Instance;
    /// @brief Everything one action passes around (case, trace, chosen branch, notices).
    class Context;

    /// @brief Reads every .json file in the templates folder and builds it; problems go to templateErrors.
    void loadTemplates();
    /// @brief Parses, builds and registers one spec.
    /// @param spec the spec as JSON
    /// @param isTemplate true if it came from a template file
    /// @param file the template file name (empty for a wizard spec)
    /// @param errors receives the problems if the spec is invalid
    /// @return the new scenario, or nullptr if the spec is invalid
    ScenarioRuntime* addScenario(const Json& spec, bool isTemplate, const std::string& file,
                                 std::vector<std::string>& errors);
    /// @brief Finds a scenario.
    /// @param key the scenario's key
    /// @return the scenario; throws GatewayError(404) if unknown
    ScenarioRuntime& scenarioOrThrow(const std::string& key);
    /// @brief Finds a case.
    /// @param id the case id
    /// @return the case; throws GatewayError(404) if unknown
    Instance& instanceOrThrow(const std::string& id);

    /// @brief Every task of a case, from WorkflowManager::getTaskIds / getTask / getItem, in tree order.
    /// @param instance the case
    /// @return the tasks
    std::vector<CaseTask> tasksOf(const Instance& instance);
    /// @brief One task of a case, from WorkflowManager::getItem / getTask.
    /// @param instance the case
    /// @param taskId the task id
    /// @return the task; node and item are null if there is none
    CaseTask taskOf(const Instance& instance, const std::string& taskId);
    /// @brief The case's root, from WorkflowManager::getTree.
    /// @param instance the case
    /// @return the root component
    WorkComponent* rootOf(const Instance& instance);
    /// @brief Checks WorkflowManager::getStatus.
    /// @param instance the case
    /// @return true once the case has been started
    bool isStarted(const Instance& instance);
    /// @brief The status shown in the GUI.
    /// @param instance the case
    /// @return "Created", "Running", or "Finished" once every task is completed, cancelled or rejected
    std::string displayStatus(const Instance& instance);
    /// @brief The size of the case's command history (WorkflowManager::getCommands()->getHistory()).
    /// @param instance the case
    /// @return the number of undoable commands
    size_t historyDepth(const Instance& instance);

    /// @brief Runs one lifecycle action on one task and builds the reply (trace, changes, notices).
    /// @param instanceId the case id
    /// @param taskId the task id
    /// @param action makeAvailable, assign, autoAssign, start, execute, complete, approve, reject, cancel or escalate
    /// @param participantId the participant for "assign" (otherwise empty)
    /// @param branch the branch chosen in the UI for "complete" (may be empty)
    /// @return the action reply
    Json act(const std::string& instanceId, const std::string& taskId, const std::string& action,
             const std::string& participantId, const std::string& branch);
    /// @brief Calls the engine for one action (WorkflowManager, the TaskRouter or the Template Method), then the follow-ups.
    /// @param instance the case
    /// @param context the action's context
    /// @param task the task
    /// @param action the action name
    /// @param participantId the participant for "assign"
    /// @param facadeRefusal receives the message if WorkflowManager refused by throwing
    /// @return true if the engine accepted the action
    bool perform(Instance& instance, Context& context, const CaseTask& task, const std::string& action,
                 const std::string& participantId, std::string& facadeRefusal);
    /// @brief Turns a WorkflowManager refusal into a message for the user.
    /// @param instance the case
    /// @param task the task
    /// @param action the action that was refused
    /// @param reason the exception text from WorkflowManager
    /// @return the message
    std::string refusalText(Instance& instance, const CaseTask& task, const std::string& action, const std::string& reason);
    /// @brief Calls execute() on the task and reads the outcome (approval chain result, automated step result).
    /// @param instance the case
    /// @param context the action's context
    /// @param task the task
    /// @return true if execute() ran
    bool runTemplateMethod(Instance& instance, Context& context, const CaseTask& task);
    /// @brief After an assignment: requests the task's resource from the case's Mediator if the spec needs one.
    /// @param instance the case
    /// @param context the action's context
    /// @param task the task
    /// @param participantId the new assignee
    void afterAssign(Instance& instance, Context& context, const CaseTask& task, const std::string& participantId);
    /// @brief Handles tasks WorkflowManager assigned by itself (start() routes tasks with a strategy).
    /// @param instance the case
    /// @param context the action's context
    /// @param before task rows before the call
    /// @param skipId a task the user assigned in this action (not counted as routed)
    void noticeRouted(Instance& instance, Context& context, const std::vector<Row>& before, const std::string& skipId);
    /// @brief After a state change: runs the decision and the spec's effects for the new state.
    /// @param instance the case
    /// @param context the action's context
    /// @param task the task
    /// @param newState the state the task moved to
    void afterTransition(Instance& instance, Context& context, const CaseTask& task, const std::string& newState);
    /// @brief Applies a task's onComplete / onReject / onCancel effects through WorkflowManager.
    /// @param instance the case
    /// @param context the action's context
    /// @param taskId the task id
    /// @param when "onComplete", "onReject" or "onCancel"
    void applyEffects(Instance& instance, Context& context, const std::string& taskId, const std::string& when);
    /// @brief Takes one branch of the decision after a task and cancels the others with WorkflowManager::cancel.
    /// @param instance the case
    /// @param context the action's context (holds the branch chosen in the UI)
    /// @param taskId the task the decision follows
    void decide(Instance& instance, Context& context, const std::string& taskId);
    /// @brief Cancels one task with WorkflowManager::cancel and records it.
    /// @param instance the case
    /// @param context the action's context
    /// @param taskId the task to cancel
    /// @param why reason shown in the trace
    /// @return true if the task was cancelled
    bool facadeCancel(Instance& instance, Context& context, const std::string& taskId, const std::string& why);
    /// @brief Hands a task's resource back with ResourceCoordinator::release and forgets it.
    /// @param instance the case
    /// @param taskId the task id
    /// @param trace the trace to record the step in
    /// @param why reason shown in the trace
    void releaseResource(Instance& instance, const std::string& taskId, Trace& trace, const std::string& why);
    /// @brief Sets each task's request value and automated-step result from the spec and the case data.
    /// @param instance the case
    /// @param trace the trace to record the step in
    void configure(Instance& instance, Trace& trace);
    /// @brief Publishes one event per changed task on the case's WorkflowEventBus (WorkflowManager publishes none).
    /// @param instance the case
    /// @param seen task states before the action; updated as events are published
    /// @param trace the trace to record the steps in
    /// @param fromUndo true to publish EXTERNAL_EVENT (an undo is not a new completion)
    void publishChanges(Instance& instance, std::vector<Row>& seen, Trace& trace, bool fromUndo);
    /// @brief Calls Participant::itemAssigned / itemReleased when a task's holder changes (no engine class does).
    /// @param instance the case
    /// @param trace the trace to record the step in, or nullptr
    void syncWorkload(Instance& instance, Trace* trace);
    /// @brief Stores the case data in the case's WorkflowInstance (WorkflowInstance::setData).
    /// @param instance the case
    void storeData(Instance& instance);
    /// @brief Records the state and assignee of every task.
    /// @param instance the case
    /// @return one row per task, in tree order
    std::vector<Row> rowsOf(const Instance& instance);

    /// @brief Builds the reply for an action: message, trace, console output, changed tasks and the case view.
    /// @param instance the case
    /// @param ok whether the action was accepted
    /// @param message the message to show
    /// @param warning true if it ran but something should be noticed (e.g. a refused resource)
    /// @param trace the steps that ran
    /// @param before task states before the action
    /// @param console the engine's std::cout output
    /// @return the reply JSON
    Json respond(Instance& instance, bool ok, const std::string& message, bool warning, Trace& trace,
                 const std::vector<Row>& before, const Json& console);
    /// @brief Builds the full view of a case (tree, data, history, audit, snapshots).
    /// @param instance the case
    /// @return the case view
    Json instanceView(Instance& instance);
    /// @brief Builds the description of a scenario for the UI.
    /// @param scenario the scenario
    /// @return the scenario view
    Json scenarioView(ScenarioRuntime& scenario);
    /// @brief Builds the view of one tree node (stage or task) and its children.
    /// @param scenario the scenario the tree belongs to
    /// @param data the case data
    /// @param node the node
    /// @param path the node's position (child indices from the root)
    /// @return the node view
    Json treeNode(ScenarioRuntime& scenario, const std::map<std::string, std::string>& data, WorkComponent* node,
                  const std::vector<int>& path);
    /// @brief Builds the short summary of a case for the case list.
    /// @param instance the case
    /// @return the summary
    Json summary(Instance& instance);

    std::mutex mutex;                                  ///< Guards every public method
    std::string templatesDir;                          ///< Folder holding the template .json files
    std::unique_ptr<DefinitionRegistry> registry;      ///< Real Prototype manager, holds every definition
    std::unique_ptr<TaskRouter> router;                ///< Real TaskRouter shared with WorkflowManager
    std::unique_ptr<WorkflowManager> manager;          ///< The Facade: owns every case's tree, mediator and commands
    std::vector<std::unique_ptr<ScenarioRuntime> > scenarioList;   ///< Every scenario, templates first
    std::map<std::string, bool> templateFlags;         ///< Scenario key -> loaded from a template file
    std::map<std::string, std::string> templateFiles;  ///< Scenario key -> template file name
    std::vector<std::string> templateErrors;           ///< Problems found while loading the templates
    std::map<std::string, std::unique_ptr<Instance> > instances;   ///< Case id -> case
    std::vector<std::string> instanceOrder;            ///< Case ids in creation order
    int resetCount;                                    ///< How many times the demo was reset
};

#endif
