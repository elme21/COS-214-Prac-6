// TEMPORARY - stands in for ConnectorManager (Member 7) until the real Adapter classes exist. Delete when that lands.
// Copied unchanged from tests/stubs/ConnectorManager.h (that folder is not part of the repository).
// STUB for Member 7's class - local testing only, never copy to include/.
#ifndef CONNECTORMANAGER_H
#define CONNECTORMANAGER_H

#include <map>
#include <string>

class Connector;

class ConnectorManager {
public:
    virtual ~ConnectorManager();
    // The UML calls this register(), which is a C++ keyword; the real name is Member 7's call.
    void registerConnector(const std::string& name, Connector* connector);
    Connector* get(const std::string& name) const;

private:
    std::map<std::string, Connector*> connectors;  // non-owning
};

#endif
