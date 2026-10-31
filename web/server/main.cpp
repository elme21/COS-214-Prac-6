// TIVIDY web demo: entry point. A small HTTP server that serves web/public/ and a JSON API.
// Every API call goes through EngineGateway, which calls the real engine.
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
    if (body.is_discarded()) throw GatewayError(400, "Request body is not valid JSON.");
    if (!body.is_object()) throw GatewayError(400, "Request body is not a JSON object.");
    return body;
}

std::string field(const json& body, const char* key) {
    json::const_iterator it = body.find(key);
    return it != body.end() && it->is_string() ? it->get<std::string>() : std::string();
}

/** Runs a handler and turns any error into a friendly JSON message (plus a list of problems). */
template <typename Handler>
void guarded(httplib::Response& res, Handler handler) {
    try {
        sendJson(res, handler());
    } catch (const GatewayError& e) {
        sendJson(res, json{{"ok", false}, {"message", e.what()}, {"errors", e.errors}}, e.status);
    } catch (const std::exception& e) {
        sendJson(res, json{{"ok", false}, {"message", std::string("Engine error: ") + e.what()}}, 500);
    } catch (...) {
        sendJson(res, json{{"ok", false}, {"message", "Engine error (unknown exception)."}}, 500);
    }
}

} // namespace

int main(int argc, char** argv) {
    std::string publicDir = "public";
    std::string templatesDir = "templates";
    int port = 8080;
    for (int i = 1; i + 1 < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--public") publicDir = argv[++i];
        else if (arg == "--templates") templatesDir = argv[++i];
        else if (arg == "--port") port = std::atoi(argv[++i]);
    }

    EngineGateway gateway(templatesDir);
    httplib::Server server;

    if (!server.set_mount_point("/", publicDir)) {
        std::cerr << "Cannot serve the web pages: folder '" << publicDir << "' not found." << std::endl;
        return 1;
    }

    server.Post("/api/reset", [&](const httplib::Request&, httplib::Response& res) {
        guarded(res, [&]() -> json { return gateway.reset(); });
    });
    server.Get("/api/templates", [&](const httplib::Request&, httplib::Response& res) {
        guarded(res, [&]() -> json { return gateway.templates(); });
    });
    server.Get("/api/scenarios", [&](const httplib::Request&, httplib::Response& res) {
        guarded(res, [&]() -> json { return gateway.scenarios(); });
    });
    server.Post("/api/scenarios/validate", [&](const httplib::Request& req, httplib::Response& res) {
        guarded(res, [&]() -> json { return gateway.validateSpec(parseBody(req)); });
    });
    server.Post("/api/scenarios", [&](const httplib::Request& req, httplib::Response& res) {
        guarded(res, [&]() -> json { return gateway.createScenario(parseBody(req)); });
    });
    server.Get("/api/resources/([a-z0-9-]+)", [&](const httplib::Request& req, httplib::Response& res) {
        guarded(res, [&]() -> json { return gateway.resources(req.matches[1]); });
    });
    server.Get("/api/instances", [&](const httplib::Request&, httplib::Response& res) {
        guarded(res, [&]() -> json { return gateway.listInstances(); });
    });
    server.Post("/api/instances", [&](const httplib::Request& req, httplib::Response& res) {
        guarded(res, [&]() -> json {
            json body = parseBody(req);
            json inputs = body.count("inputs") ? body["inputs"] : json::object();
            return gateway.createInstance(field(body, "scenario"), inputs, field(body, "label"));
        });
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
    server.Post("/api/instances/([A-Za-z0-9-]+)/tasks/([A-Za-z0-9_]+)/([A-Za-z]+)",
                [&](const httplib::Request& req, httplib::Response& res) {
        guarded(res, [&]() -> json {
            json body = parseBody(req);
            std::string id = req.matches[1];
            std::string task = req.matches[2];
            std::string action = req.matches[3];
            if (action == "makeAvailable") return gateway.makeAvailable(id, task);
            if (action == "assign") return gateway.assignTo(id, task, field(body, "participant"));
            if (action == "autoAssign") return gateway.autoAssign(id, task);
            if (action == "start") return gateway.startTask(id, task);
            if (action == "execute") return gateway.execute(id, task);
            if (action == "complete") return gateway.complete(id, task, field(body, "branch"));
            if (action == "approve") return gateway.approve(id, task);
            if (action == "reject") return gateway.reject(id, task);
            if (action == "cancel") return gateway.cancel(id, task);
            if (action == "escalate") return gateway.escalate(id, task);
            throw GatewayError(400, "Unknown action '" + action + "'");
        });
    });

    // An API path or method that matches no route still gets a JSON message (handled replies keep theirs).
    httplib::Server::HandlerWithResponse apiError = [](const httplib::Request& req, httplib::Response& res) {
        if (!res.body.empty() || req.path.compare(0, 5, "/api/") != 0) return httplib::Server::HandlerResponse::Unhandled;
        sendJson(res, json{{"ok", false}, {"message", "No API endpoint " + req.method + " " + req.path.substr(0, 200) + "."}}, res.status);
        return httplib::Server::HandlerResponse::Handled;
    };
    server.set_error_handler(apiError);

    std::cout << "TIVIDY web demo running at http://localhost:" << port << "/  (Ctrl+C to stop)" << std::endl;
    std::cout << "Serving pages from: " << publicDir << "; templates from: " << templatesDir << std::endl;
    if (!server.listen("127.0.0.1", port)) {
        std::cerr << "Could not listen on port " << port << " (is another copy already running?)" << std::endl;
        return 1;
    }
    return 0;
}
