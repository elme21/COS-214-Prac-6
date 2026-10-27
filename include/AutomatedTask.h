#ifndef AUTOMATEDTASK_H
#define AUTOMATEDTASK_H

#include "WorkItem.h"

#include <string>

class ConnectorManager;

/** A task the system runs itself by calling an external connector. */
class AutomatedTask : public WorkItem {
public:
    AutomatedTask();
    AutomatedTask(const std::string& connectorName, ConnectorManager* connectors);
    explicit AutomatedTask(const std::string& name,
                           const std::string& id = std::string(),
                           const std::string& connectorName = std::string(),
                           ConnectorManager* connectors = nullptr);

    void setConnectorName(const std::string& connectorName);
    void setConnectorManager(ConnectorManager* connectors);

    // Prototype - coordinate with Member 3
    WorkComponent* clone() const override;

protected:
    bool perform() override;

private:
    bool callConnector();

    std::string connectorName;
    // NON-OWNING: ConnectorManager is owned by the Facade / Adapter layer (Member 7).
    ConnectorManager* connectors;
};

#endif
