// TEMPORARY - stands in for ConnectorManager (Member 7) until the real Adapter classes exist. Delete when that lands.
// Copied unchanged from tests/stubs/ConnectorManager.cpp (that folder is not part of the repository).
#include "ConnectorManager.h"

ConnectorManager::~ConnectorManager() {}

void ConnectorManager::registerConnector(const std::string& name, Connector* connector) {
    connectors[name] = connector;
}

Connector* ConnectorManager::get(const std::string& name) const {
    std::map<std::string, Connector*>::const_iterator it = connectors.find(name);
    return it == connectors.end() ? nullptr : it->second;
}
