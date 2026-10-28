#ifndef AUTOMATEDTASK_H
#define AUTOMATEDTASK_H

#include "WorkItem.h"

#include <string>

class ConnectorManager;

/// @brief
/// The AutomatedTask class is a ConcreteClass of the Template Method pattern (and a Composite leaf):
/// a task the system runs itself by calling an external connector. It is the Client of the Adapter pattern.
class AutomatedTask : public WorkItem {
public:
    /// @brief The default constructor. No connector name and no connector manager.
    AutomatedTask();
    /// @brief The constructor (no name or ID)
    /// @param connectorName the name of the connector to call, looked up in the connector manager
    /// @param connectors the connector manager (non-owning)
    AutomatedTask(const std::string& connectorName, ConnectorManager* connectors);
    /// @brief The constructor
    /// @param name the display name of the task
    /// @param id the ID of the task within its definition
    /// @param connectorName the name of the connector to call, looked up in the connector manager
    /// @param connectors the connector manager (non-owning)
    explicit AutomatedTask(const std::string& name,
                           const std::string& id = std::string(),
                           const std::string& connectorName = std::string(),
                           ConnectorManager* connectors = nullptr);

    /// @brief sets the name of the connector to call
    /// @param connectorName the connector name
    void setConnectorName(const std::string& connectorName);
    /// @brief sets the connector manager
    /// @param connectors the connector manager (non-owning)
    void setConnectorManager(ConnectorManager* connectors);

    // Prototype - coordinate with Member 3
    /// @brief creates a copy of this task (Prototype), including its state, assignee, name, ID, connector
    /// name and the same (non-owned) connector manager pointer
    /// @return a new AutomatedTask owned by the caller
    WorkComponent* clone() const override;

protected:
    /// @brief the primitive operation: prints the connector name and calls the connector
    /// @return true if the connector reported success (the item then completes), false otherwise
    /// (the item stays InProgress)
    bool perform() override;

private:
    /// @brief looks up the connector by name in the connector manager and executes it
    /// @return true if the connector reported success; false if there is no connector manager,
    /// no connector with that name, or the connector failed
    bool callConnector();

    /// @brief The name of the connector to call
    std::string connectorName;
    // NON-OWNING: ConnectorManager is owned by the Facade / Adapter layer (Member 7).
    /// @brief The connector manager, or nullptr. NON-OWNING: owned by the Facade / Adapter layer (Member 7).
    ConnectorManager* connectors;
};

#endif
