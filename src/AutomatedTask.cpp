#include "AutomatedTask.h"
#include "ConnectorManager.h"
#include "Connector.h"

#include <iostream>

AutomatedTask::AutomatedTask() : connectorName(), connectors(nullptr) {}

AutomatedTask::AutomatedTask(const std::string& name, ConnectorManager* manager)
    : connectorName(name), connectors(manager) {}

AutomatedTask::AutomatedTask(const std::string& name, const std::string& id,
                             const std::string& connector, ConnectorManager* manager)
    : WorkItem(name, id), connectorName(connector), connectors(manager) {}

void AutomatedTask::setConnectorName(const std::string& name) { connectorName = name; }
void AutomatedTask::setConnectorManager(ConnectorManager* manager) { connectors = manager; }

WorkComponent* AutomatedTask::clone() const { return new AutomatedTask(*this); }

bool AutomatedTask::perform() {
    std::cout << "[AutomatedTask] calling connector '" << connectorName << "'" << std::endl;
    return callConnector();
}

// The only place that touches Member 7's connector types.
// TODO align types with Member 7 (ConnectorManager::get, ConnectorRequest, ConnectorResult)
bool AutomatedTask::callConnector() {
    if (connectors == nullptr) return false;
    Connector* connector = connectors->get(connectorName);
    if (connector == nullptr) return false;
    ConnectorRequest request;
    ConnectorResult result = connector->execute(request);
    return result.isSuccess();
}
