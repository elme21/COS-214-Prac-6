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
class WorkflowCommand;
class DefinitionRegistry;
class ScenarioRuntime;
struct TaskRef;

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

/**
 * @brief The ONLY place the web server touches the TIVIDY engine: JSON in, engine calls, JSON out.
 *
 * Completely generic: every scenario is a ScenarioSpec (loaded from the .json files in web/templates or
 * created in the wizard), built by the GenericDirector on the real CodeDefinitionBuilder and
 * registered in the real DefinitionRegistry. Nothing here knows about any industry.
 *
 * Public methods mirror WorkflowManager (createInstance, start, complete, getStatus) plus the
 * other actions a marker needs. WorkflowManager itself cannot be used by a GUI: it keeps every
 * instance's tree, mediator and command processor private, and its complete() needs an InProgress
 * task that no Facade operation can produce (see INTEGRATION.md). So the internals do the Facade's
 * job with the real classes: clone from the registry, run the real commands through a real
 * CommandProcessor, wire a real ResourceCoordinator, route with the real TaskRouter, and publish
 * real Events whose real listeners react.
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
    /// @brief Deletes every instance before the scenarios and the registry they use.
    ~EngineGateway();

    /// @brief Discards every instance and custom scenario, then reloads the templates.
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
    /// @brief Summarises every instance (status, outcome, state counts).
    /// @return an array of summaries
    Json listInstances();
    /// @brief Lists a scenario's resources with capacity, free places and holders.
    /// @param scenarioKey the scenario's key
    /// @return an array of resources
    Json resources(const std::string& scenarioKey);

    /// @brief Mirrors WorkflowManager::createInstance: clones the definition and wires a new instance.
    /// @param scenarioKey the scenario's key
    /// @param inputs input field -> value for the new case
    /// @param label optional title; the subject field's value is used if empty
    /// @return the action reply with the new instance view
    Json createInstance(const std::string& scenarioKey, const Json& inputs, const std::string& label);
    /// @brief Mirrors WorkflowManager::start: makes every task with no open prerequisite available.
    /// @param instanceId the instance id
    /// @return the action reply
    Json start(const std::string& instanceId);
    /// @brief Mirrors WorkflowManager::complete: runs a CompleteCommand on the task.
    /// @param instanceId the instance id
    /// @param taskId the task id
    /// @param branch for a task followed by a decision: the branch chosen in the UI (may be empty)
    /// @return the action reply
    Json complete(const std::string& instanceId, const std::string& taskId, const std::string& branch);
    /// @brief Mirrors WorkflowManager::getStatus, with the full instance view.
    /// @param instanceId the instance id
    /// @return the instance view
    Json getStatus(const std::string& instanceId);

    /// @brief Runs a MakeAvailableCommand on a task.
    /// @param instanceId the instance id
    /// @param taskId the task id
    /// @return the action reply
    Json makeAvailable(const std::string& instanceId, const std::string& taskId);
    /// @brief Runs an AssignCommand, then asks the Mediator for the task's resource if it needs one.
    /// @param instanceId the instance id
    /// @param taskId the task id
    /// @param participantId the participant to assign
    /// @return the action reply
    Json assignTo(const std::string& instanceId, const std::string& taskId, const std::string& participantId);
    /// @brief Routes a task with the real TaskRouter and the strategy its spec names.
    /// @param instanceId the instance id
    /// @param taskId the task id
    /// @return the action reply
    Json autoAssign(const std::string& instanceId, const std::string& taskId);
    /// @brief Runs a StartCommand on a task (refused if it needs a resource it does not hold).
    /// @param instanceId the instance id
    /// @param taskId the task id
    /// @return the action reply
    Json startTask(const std::string& instanceId, const std::string& taskId);
    /// @brief Calls WorkItem::execute() (Template Method) on a task.
    /// @param instanceId the instance id
    /// @param taskId the task id
    /// @return the action reply
    Json execute(const std::string& instanceId, const std::string& taskId);
    /// @brief Runs an ApproveCommand on a task.
    /// @param instanceId the instance id
    /// @param taskId the task id
    /// @return the action reply
    Json approve(const std::string& instanceId, const std::string& taskId);
    /// @brief Runs a RejectCommand on a task.
    /// @param instanceId the instance id
    /// @param taskId the task id
    /// @return the action reply
    Json reject(const std::string& instanceId, const std::string& taskId);
    /// @brief Runs a CancelCommand on a task.
    /// @param instanceId the instance id
    /// @param taskId the task id
    /// @return the action reply
    Json cancel(const std::string& instanceId, const std::string& taskId);
    /// @brief Runs an EscalateCommand on a task.
    /// @param instanceId the instance id
    /// @param taskId the task id
    /// @return the action reply
    Json escalate(const std::string& instanceId, const std::string& taskId);

    /// @brief Undoes the newest command with CommandProcessor::undo().
    /// @param instanceId the instance id
    /// @return the action reply
    Json undo(const std::string& instanceId);
    /// @brief Saves a snapshot with WorkflowInstance::createSnapshot() and SnapshotHistory::save().
    /// @param instanceId the instance id
    /// @return the action reply
    Json snapshot(const std::string& instanceId);
    /// @brief Restores the newest snapshot with WorkflowInstance::restoreSnapshot() and rewires the new tree.
    /// @param instanceId the instance id
    /// @return the action reply
    Json restoreSnapshot(const std::string& instanceId);

    /// @brief The state of one task at a moment, used to find what an action changed.
    struct Row;

private:
    /// @brief One running instance: its real engine objects plus display bookkeeping.
    struct Instance;
    /// @brief Everything one action passes around (instance, trace, chosen branch, notices).
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
    /// @brief Finds an instance.
    /// @param id the instance id
    /// @return the instance; throws GatewayError(404) if unknown
    Instance& instanceOrThrow(const std::string& id);

    /// @brief Runs one lifecycle action on one task and builds the reply (trace, changes, notices).
    /// @param instanceId the instance id
    /// @param taskId the task id
    /// @param action makeAvailable, assign, autoAssign, start, execute, complete, approve, reject, cancel or escalate
    /// @param participantId the participant for "assign" (otherwise empty)
    /// @param branch the branch chosen in the UI for "complete" (may be empty)
    /// @return the action reply
    Json act(const std::string& instanceId, const std::string& taskId, const std::string& action,
             const std::string& participantId, const std::string& branch);
    /// @brief Calls the engine for one action (command, router or Template Method), then the follow-ups.
    /// @param instance the instance
    /// @param context the action's context
    /// @param task the task
    /// @param action the action name
    /// @param participantId the participant for "assign"
    /// @return true if the engine accepted the action
    bool perform(Instance& instance, Context& context, const TaskRef& task, const std::string& action,
                 const std::string& participantId);
    /// @brief Calls WorkItem::execute() and reads the outcome (approval chain result, automated step result).
    /// @param instance the instance
    /// @param context the action's context
    /// @param task the task
    /// @return true if execute() ran
    bool runTemplateMethod(Instance& instance, Context& context, const TaskRef& task);
    /// @brief After an assignment: requests the task's resource from the Mediator if the spec needs one.
    /// @param instance the instance
    /// @param context the action's context
    /// @param task the task
    /// @param participantId the new assignee
    void afterAssign(Instance& instance, Context& context, const TaskRef& task, const std::string& participantId);
    /// @brief After a state change: runs the decision and the spec's effects for the new state.
    /// @param instance the instance
    /// @param context the action's context
    /// @param task the task
    /// @param newState the state the task moved to
    void afterTransition(Instance& instance, Context& context, const TaskRef& task, const std::string& newState);
    /// @brief Applies a task's onComplete / onReject / onCancel effects with real commands and Mediator calls.
    /// @param instance the instance
    /// @param context the action's context
    /// @param taskId the task id
    /// @param when "onComplete", "onReject" or "onCancel"
    void applyEffects(Instance& instance, Context& context, const std::string& taskId, const std::string& when);
    /// @brief Takes one branch of the decision after a task and cancels the others with CancelCommands.
    /// @param instance the instance
    /// @param context the action's context (holds the branch chosen in the UI)
    /// @param taskId the task the decision follows
    void decide(Instance& instance, Context& context, const std::string& taskId);
    /// @brief Runs a command through the instance's CommandProcessor and records its label for the history.
    /// @param instance the instance
    /// @param command the command (the processor takes ownership)
    /// @param label display text for the history
    /// @return the command's execute() result
    bool runCommand(Instance& instance, WorkflowCommand* command, const std::string& label);
    /// @brief Hands a task's resource back with ResourceCoordinator::release and forgets it.
    /// @param instance the instance
    /// @param taskId the task id
    /// @param trace the trace to record the step in
    /// @param why reason shown in the trace
    void releaseResource(Instance& instance, const std::string& taskId, Trace& trace, const std::string& why);
    /// @brief Registers every task with the instance's ResourceCoordinator and adds the definition's dependencies.
    /// @param instance the instance
    /// @param trace the trace to record the step in
    void wire(Instance& instance, Trace& trace);
    /// @brief Sets each task's request value and automated-step result from the spec and the case data.
    /// @param instance the instance
    /// @param trace the trace to record the step in
    void configure(Instance& instance, Trace& trace);
    /// @brief Publishes one event per changed task on the instance's WorkflowEventBus.
    /// @param instance the instance
    /// @param seen task states before the action; updated as events are published
    /// @param trace the trace to record the steps in
    /// @param fromUndo true to publish EXTERNAL_EVENT (an undo is not a new completion)
    void publishChanges(Instance& instance, std::vector<Row>& seen, Trace& trace, bool fromUndo);
    /// @brief Calls Participant::itemAssigned / itemReleased when a task's holder changes (no engine class does).
    /// @param instance the instance
    /// @param trace the trace to record the step in, or nullptr
    void syncWorkload(Instance& instance, Trace* trace);
    /// @brief Sets the instance status (Created / Running / Finished) and stores the case data in WorkflowInstance.
    /// @param instance the instance
    void updateStatus(Instance& instance);
    /// @brief Records the state and assignee of every task.
    /// @param root the instance's root
    /// @return one row per task, depth-first
    static std::vector<Row> rowsOf(WorkComponent* root);

    /// @brief Builds the reply for an action: message, trace, console output, changed tasks and the instance view.
    /// @param instance the instance
    /// @param ok whether the action was accepted
    /// @param message the message to show
    /// @param warning true if it ran but something should be noticed (e.g. a refused resource)
    /// @param trace the steps that ran
    /// @param before task states before the action
    /// @param console the engine's std::cout output
    /// @return the reply JSON
    Json respond(Instance& instance, bool ok, const std::string& message, bool warning, Trace& trace,
                 const std::vector<Row>& before, const Json& console);
    /// @brief Builds the full view of an instance (tree, data, history, audit, snapshots).
    /// @param instance the instance
    /// @return the instance view
    Json instanceView(Instance& instance);
    /// @brief Builds the description of a scenario for the UI.
    /// @param scenario the scenario
    /// @return the scenario view
    Json scenarioView(ScenarioRuntime& scenario);
    /// @brief Builds the view of one tree node (stage or task) and its children.
    /// @param scenario the scenario the tree belongs to
    /// @param data the instance's case data
    /// @param node the node
    /// @param path the node's position (child indices from the root)
    /// @return the node view
    Json treeNode(ScenarioRuntime& scenario, const std::map<std::string, std::string>& data, WorkComponent* node,
                  const std::vector<int>& path);
    /// @brief Builds the short summary of an instance for the instance list.
    /// @param instance the instance
    /// @return the summary
    Json summary(Instance& instance);

    std::mutex mutex;                                  ///< Guards every public method
    std::string templatesDir;                          ///< Folder holding the template .json files
    std::unique_ptr<DefinitionRegistry> registry;      ///< Real Prototype manager, holds every definition
    std::vector<std::unique_ptr<ScenarioRuntime> > scenarioList;   ///< Every scenario, templates first
    std::map<std::string, bool> templateFlags;         ///< Scenario key -> loaded from a template file
    std::map<std::string, std::string> templateFiles;  ///< Scenario key -> template file name
    std::vector<std::string> templateErrors;           ///< Problems found while loading the templates
    std::map<std::string, std::unique_ptr<Instance> > instances;   ///< Instance id -> instance
    std::vector<std::string> instanceOrder;            ///< Instance ids in creation order
    int resetCount;                                    ///< How many times the demo was reset
};

#endif
