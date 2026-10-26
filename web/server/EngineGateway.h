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

/** An error the HTTP layer turns into a status code and a friendly message. */
class GatewayError : public std::runtime_error {
public:
    GatewayError(int httpStatus, const std::string& message) : std::runtime_error(message), status(httpStatus) {}
    int status;
};

/**
 * @brief The ONLY place the web server touches the TIVIDY engine.
 *
 * Public methods mirror WorkflowManager (createInstance, start, complete,
 * getStatus) plus the other actions a marker needs. WorkflowManager's methods
 * all throw "waiting for integration" today, so the internals call the working
 * engine classes directly. When the Facade works, only this class's internals
 * change: main.cpp and the browser code stay as they are (see INTEGRATION.md).
 *
 * Every response carries a "trace" (what ran, for the Patterns panel), the
 * engine's own console output, and the updated instance.
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

    // --- the other lifecycle actions (WorkItem / WorkComponent operations) ---
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
    class Context;

    void build();
    Industry& industryOrThrow(const std::string& key);
    Instance& instanceOrThrow(const std::string& id);

    Json act(const std::string& instanceId, const std::string& taskId, const std::string& action,
             const std::string& participantId, const std::string& outcome);
    bool perform(Instance& instance, Context& context, const std::string& action,
                 const std::string& participantId, const std::string& outcome);
    bool runTemplateMethod(Instance& instance, Context& context);
    void undoSideEffects(Instance& instance, const std::map<std::string, std::string>& dataBefore);
    void wire(Instance& instance, Trace& trace);
    void updateStatus(Instance& instance);

    Json instanceView(Instance& instance);
    Json treeNode(Industry& industry, const std::map<std::string, std::string>& data, WorkComponent* node,
                  const std::vector<int>& path);
    Json summary(Instance& instance);

    std::mutex mutex;
    std::map<std::string, std::unique_ptr<Industry> > industries;
    std::vector<std::string> industryOrder;
    std::map<std::string, std::unique_ptr<Instance> > instances;
    std::vector<std::string> instanceOrder;
    Trace* activeTrace;   // set while CommandProcessor::undo() runs
    int resetCount;
};

#endif
