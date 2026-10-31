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

/** An error the HTTP layer turns into a status code and a friendly message (plus optional details). */
class GatewayError : public std::runtime_error {
public:
    GatewayError(int httpStatus, const std::string& message,
                 const std::vector<std::string>& details = std::vector<std::string>())
        : std::runtime_error(message), status(httpStatus), errors(details) {}
    int status;
    std::vector<std::string> errors;
};

/**
 * @brief The ONLY place the web server touches the TIVIDY engine: JSON in, engine calls, JSON out.
 *
 * Completely generic: every scenario is a ScenarioSpec (loaded from the .json files in web/templates or
 * created in the editor), built by the GenericDirector on the real CodeDefinitionBuilder and
 * registered in the real DefinitionRegistry. Nothing here knows about any industry.
 *
 * Public methods mirror WorkflowManager (createInstance, start, complete, getStatus) plus the
 * other actions a marker needs. WorkflowManager itself cannot be used by a GUI: it keeps every
 * instance's tree, mediator and command processor private (see INTEGRATION.md). So the internals
 * do the Facade's job with the real classes: clone from the registry, run the real commands
 * through a real CommandProcessor, wire a real ResourceCoordinator, route with the real
 * TaskRouter, and publish real Events whose real listeners react.
 *
 * Thread safety: one mutex around every public method (httplib uses a thread pool).
 */
class EngineGateway {
public:
    typedef nlohmann::json Json;

    explicit EngineGateway(const std::string& templatesDir);
    ~EngineGateway();

    Json meta();
    Json reset();
    Json templates();
    Json scenarios();
    Json validateSpec(const Json& spec);
    Json createScenario(const Json& spec);
    Json listInstances();
    Json resources(const std::string& scenarioKey);

    // --- mirrors WorkflowManager ---
    Json createInstance(const std::string& scenarioKey, const Json& inputs, const std::string& label);
    Json start(const std::string& instanceId);
    Json complete(const std::string& instanceId, const std::string& taskId, const std::string& branch);
    Json getStatus(const std::string& instanceId);

    // --- the other lifecycle actions (each runs the matching real command) ---
    Json makeAvailable(const std::string& instanceId, const std::string& taskId);
    Json assignTo(const std::string& instanceId, const std::string& taskId, const std::string& participantId);
    Json autoAssign(const std::string& instanceId, const std::string& taskId);
    Json startTask(const std::string& instanceId, const std::string& taskId);
    Json execute(const std::string& instanceId, const std::string& taskId);
    Json approve(const std::string& instanceId, const std::string& taskId);
    Json reject(const std::string& instanceId, const std::string& taskId);
    Json cancel(const std::string& instanceId, const std::string& taskId);
    Json escalate(const std::string& instanceId, const std::string& taskId);

    // --- Command undo and Memento ---
    Json undo(const std::string& instanceId);
    Json snapshot(const std::string& instanceId);
    Json restoreSnapshot(const std::string& instanceId);

    struct Row;

private:
    struct Instance;
    class Context;

    void loadTemplates();
    ScenarioRuntime* addScenario(const Json& spec, bool isTemplate, const std::string& file,
                                 std::vector<std::string>& errors);
    ScenarioRuntime& scenarioOrThrow(const std::string& key);
    Instance& instanceOrThrow(const std::string& id);

    Json act(const std::string& instanceId, const std::string& taskId, const std::string& action,
             const std::string& participantId, const std::string& branch);
    bool perform(Instance& instance, Context& context, const TaskRef& task, const std::string& action,
                 const std::string& participantId);
    bool runTemplateMethod(Instance& instance, Context& context, const TaskRef& task);
    void afterAssign(Instance& instance, Context& context, const TaskRef& task, const std::string& participantId);
    void afterTransition(Instance& instance, Context& context, const TaskRef& task, const std::string& newState);
    void applyEffects(Instance& instance, Context& context, const std::string& taskId, const std::string& when);
    void decide(Instance& instance, Context& context, const std::string& taskId);
    bool runCommand(Instance& instance, WorkflowCommand* command, const std::string& label);
    void releaseResource(Instance& instance, const std::string& taskId, Trace& trace, const std::string& why);
    void wire(Instance& instance, Trace& trace);
    void configure(Instance& instance, Trace& trace);
    void publishChanges(Instance& instance, std::vector<Row>& seen, Trace& trace, bool fromUndo);
    void syncWorkload(Instance& instance, Trace* trace);
    void updateStatus(Instance& instance);
    static std::vector<Row> rowsOf(WorkComponent* root);

    Json respond(Instance& instance, bool ok, const std::string& message, bool warning, Trace& trace,
                 const std::vector<Row>& before, const Json& console);
    Json instanceView(Instance& instance);
    Json scenarioView(ScenarioRuntime& scenario);
    Json treeNode(ScenarioRuntime& scenario, const std::map<std::string, std::string>& data, WorkComponent* node,
                  const std::vector<int>& path);
    Json summary(Instance& instance);

    std::mutex mutex;
    std::string templatesDir;
    std::unique_ptr<DefinitionRegistry> registry;   // real Prototype manager, holds every definition
    std::vector<std::unique_ptr<ScenarioRuntime> > scenarioList;
    std::map<std::string, bool> templateFlags;      // scenario key -> loaded from a template file
    std::map<std::string, std::string> templateFiles;
    std::vector<std::string> templateErrors;
    std::map<std::string, std::unique_ptr<Instance> > instances;
    std::vector<std::string> instanceOrder;
    int resetCount;
};

#endif
