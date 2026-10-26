// TEMPORARY - stands in for Connector (Member 7) until the real Adapter classes exist. Delete when that lands.
// Copied unchanged from tests/stubs/Connector.h (that folder is not part of the repository).
// STUB for Member 7's classes - local testing only, never copy to include/.
#ifndef CONNECTOR_H
#define CONNECTOR_H

class ConnectorRequest {};

class ConnectorResult {
public:
    explicit ConnectorResult(bool success = false);
    bool isSuccess() const;

private:
    bool success;
};

class Connector {
public:
    virtual ~Connector();
    virtual ConnectorResult execute(const ConnectorRequest& request) = 0;
};

#endif
