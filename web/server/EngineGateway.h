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
struct TaskRef;

/** An error the HTTP layer turns into a status code and a friendly message. */
class GatewayError : public std::runtime_error {
public:
    GatewayError(int httpStatus, const std::string& message) : std::runtime_error(message), status(httpStatus) {}
    int status;
};

/**
 * @brief The ONLY place the web server touches the TIVIDY engine: JSON in, engine calls, JSON out.
 *
 * Public methods mirror WorkflowManager (createInstance, start, complete, getStatus) plus the
 * other actions a marker needs. The real WorkflowManager cannot be used yet (it does not link
 * without TaskRouter, and it has no way to reach an instance's tasks), so the internals do what
 * the Facade would: clone from the real DefinitionRegistry, run the real commands through a real
 * CommandProcessor, wire a real ResourceCoordinator, and publish real Events on a
 * WorkflowEventBus whose real listeners (DependencyActivator, AuditLogger, ...) react.
 * When the Facade works, only this class's internals change (see INTEGRATION.md).
 *
 * Every response carries a "trace" (what ran, for the Patterns panel), the engine's own
 * console output, the state changes, and the updated instance.
 *
 * Thread safety: one mutex around every public method (httplib uses a thread pool).
 */
class EngineGateway {
public:
    typedef nlohmann::json Json;

    EngineGateway();
    ~EngineGateway();

    Json meta();
    Json reset();
    Json listInstances();
    Json resources(const std::string& industry);

    // --- mirrors WorkflowManager ---
    Json createInstance(const std::string& industry, const std::string& subjectId);
    Json start(const std::string& instanceId);
    Json complete(const std::string& instanceId, const std::string& taskId, const std::string& outcome);
    Json getStatus(const std::string& instanceId);

    // --- the other lifecycle actions (each runs the matching real command) ---
    Json makeAvailable(const std::string& instanceId, const std::string& taskId);
    Json assignTo(const std::string& instanceId, const std::string& taskId, const std::string& participantId);
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

private:
    struct Industry;
    struct Instance;
    struct Row;
    class Context;

    void build();
    Industry& industryOrThrow(const std::string& key);
    Instance& instanceOrThrow(const std::string& id);

    Json act(const std::string& instanceId, const std::string& taskId, const std::string& action,
             const std::string& participantId, const std::string& outcome);
    bool perform(Instance& instance, Context& context, const TaskRef& task, const std::string& action,
                 const std::string& participantId, const std::string& outcome);
    bool runTemplateMethod(Instance& instance, Context& context, const TaskRef& task);
    bool runCommand(Instance& instance, WorkflowCommand* command, const std::string& label);
    void wire(Instance& instance, Trace& trace);
    void publishChanges(Instance& instance, std::vector<Row>& seen, Trace& trace, bool fromUndo);
    void updateStatus(Instance& instance);
    static std::vector<Row> rowsOf(WorkComponent* root);

    Json respond(Instance& instance, bool ok, const std::string& message, bool warning, Trace& trace,
                 const std::vector<Row>& before, const Json& console);
    Json instanceView(Instance& instance);
    Json treeNode(Industry& industry, const std::map<std::string, std::string>& data, WorkComponent* node,
                  const std::vector<int>& path);
    Json summary(Instance& instance);

    std::mutex mutex;
    std::unique_ptr<DefinitionRegistry> registry;   // real Prototype manager, holds both definitions
    std::map<std::string, std::unique_ptr<Industry> > industries;
    std::vector<std::string> industryOrder;
    std::map<std::string, std::unique_ptr<Instance> > instances;
    std::vector<std::string> instanceOrder;
    int resetCount;
};

#endif
