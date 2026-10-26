// TIVIDY web demo: entry point. A small HTTP server that serves web/public/ and a
// JSON API. Every API call goes through EngineGateway, which calls the real engine.
#include "EngineGateway.h"

#include "httplib.h"
#include "json.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

using nlohmann::json;

namespace {

void sendJson(httplib::Response& res, const json& body, int status = 200) {
    res.status = status;
    res.set_header("Cache-Control", "no-store");
    res.set_content(body.dump(), "application/json; charset=utf-8");
}

json parseBody(const httplib::Request& req) {
    if (req.body.empty()) return json::object();
    json body = json::parse(req.body, nullptr, false);
    if (body.is_discarded() || !body.is_object()) throw GatewayError(400, "Request body is not a JSON object.");
    return body;
}

std::string field(const json& body, const char* key) {
    json::const_iterator it = body.find(key);
    return it != body.end() && it->is_string() ? it->get<std::string>() : std::string();
}

/** Runs a handler and turns any error into a friendly JSON message. */
template <typename Handler>
void guarded(httplib::Response& res, Handler handler) {
    try {
        sendJson(res, handler());
    } catch (const GatewayError& e) {
        sendJson(res, json{{"ok", false}, {"message", e.what()}}, e.status);
    } catch (const std::exception& e) {
        sendJson(res, json{{"ok", false}, {"message", std::string("Engine error: ") + e.what()}}, 500);
    }
}

} // namespace

int main(int argc, char** argv) {
    std::string publicDir = "public";
    int port = 8080;
    for (int i = 1; i + 1 < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--public") publicDir = argv[++i];
        else if (arg == "--port") port = std::atoi(argv[++i]);
    }

    EngineGateway gateway;
    httplib::Server server;

    if (!server.set_mount_point("/", publicDir)) {
        std::cerr << "Cannot serve the web pages: folder '" << publicDir << "' not found." << std::endl;
        return 1;
    }

    server.Get("/api/meta", [&](const httplib::Request&, httplib::Response& res) {
        guarded(res, [&]() -> json { return gateway.meta(); });
    });
    server.Post("/api/reset", [&](const httplib::Request&, httplib::Response& res) {
        guarded(res, [&]() -> json { return gateway.reset(); });
    });
    server.Get("/api/instances", [&](const httplib::Request&, httplib::Response& res) {
        guarded(res, [&]() -> json { return gateway.listInstances(); });
    });
    server.Post("/api/instances", [&](const httplib::Request& req, httplib::Response& res) {
        guarded(res, [&]() -> json {
            json body = parseBody(req);
            return gateway.createInstance(field(body, "industry"), field(body, "subject"));
        });
    });
    server.Get("/api/resources/([a-z]+)", [&](const httplib::Request& req, httplib::Response& res) {
        guarded(res, [&]() -> json { return gateway.resources(req.matches[1]); });
    });
    server.Get("/api/instances/([A-Za-z0-9-]+)", [&](const httplib::Request& req, httplib::Response& res) {
        guarded(res, [&]() -> json { return gateway.getStatus(req.matches[1]); });
    });
    server.Post("/api/instances/([A-Za-z0-9-]+)/(start|undo|snapshot|restore)",
                [&](const httplib::Request& req, httplib::Response& res) {
        guarded(res, [&]() -> json {
            std::string id = req.matches[1];
            std::string op = req.matches[2];
            if (op == "start") return gateway.start(id);
            if (op == "undo") return gateway.undo(id);
            if (op == "snapshot") return gateway.snapshot(id);
            return gateway.restoreSnapshot(id);
        });
    });
    server.Post("/api/instances/([A-Za-z0-9-]+)/tasks/([A-Za-z0-9]+)/([A-Za-z]+)",
                [&](const httplib::Request& req, httplib::Response& res) {
        guarded(res, [&]() -> json {
            json body = parseBody(req);
            std::string id = req.matches[1];
            std::string task = req.matches[2];
            std::string action = req.matches[3];
            if (action == "makeAvailable") return gateway.makeAvailable(id, task);
            if (action == "assign") return gateway.assignTo(id, task, field(body, "participant"));
            if (action == "start") return gateway.startTask(id, task);
            if (action == "execute") return gateway.execute(id, task);
            if (action == "complete") return gateway.complete(id, task, field(body, "outcome"));
            if (action == "approve") return gateway.approve(id, task);
            if (action == "reject") return gateway.reject(id, task);
            if (action == "cancel") return gateway.cancel(id, task);
            if (action == "escalate") return gateway.escalate(id, task);
            throw GatewayError(400, "Unknown action '" + action + "'");
        });
    });

    std::cout << "TIVIDY web demo running at http://localhost:" << port << "/  (Ctrl+C to stop)" << std::endl;
    std::cout << "Serving pages from: " << publicDir << std::endl;
    if (!server.listen("127.0.0.1", port)) {
        std::cerr << "Could not listen on port " << port << " (is another copy already running?)" << std::endl;
        return 1;
    }
    return 0;
}
