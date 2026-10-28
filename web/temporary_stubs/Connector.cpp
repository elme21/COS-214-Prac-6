// TEMPORARY - stands in for Connector (Member 7) until the real Adapter classes exist. Delete when that lands.
// Copied unchanged from tests/stubs/Connector.cpp (that folder is not part of the repository).
#include "Connector.h"

ConnectorResult::ConnectorResult(bool s) : success(s) {}
bool ConnectorResult::isSuccess() const { return success; }
Connector::~Connector() {}
