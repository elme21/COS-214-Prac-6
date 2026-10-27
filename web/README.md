# TIVIDY web demo

A locally run website that drives the **real TIVIDY C++11 engine** for two industries:

1. **Education**: university application and admission (the team's `EnrolmentDirector`).
2. **Healthcare**: patient appointment booking at a clinic: urgency routing, competition for
   limited doctor slots, and a referral approval chain.

The browser contains no workflow logic. Every button calls a small C++ HTTP server
(`web/server/`). The server compiles and links the engine sources in `../src` and `../include`
unchanged, and calls them through one class, `EngineGateway`. This meets spec section 5: a GUI
"must still call the C++ engine".

Everything runs **offline**: no CDNs, fonts or downloads at run time. All state is in the server's
memory, so restarting the server resets the demo.

## Requirements

- Windows with **Windows PowerShell 5.1**.
- **MSYS2 UCRT64 g++** (tested with g++ 14.2.0) on `PATH`, e.g. `C:\msys64\ucrt64\bin`.
  Build from PowerShell, not Git Bash: Git Bash's linker fails on this machine.
- A browser.

## Build and run

From the project folder:

```powershell
.\web\run.ps1
```

`run.ps1` builds if needed, starts the server and opens **http://localhost:8080**. Stop it with
**Ctrl+C**. Options: `-Rebuild`, `-NoBrowser`, `-Port 8081`.

Build only (incremental; `-Clean` for a full rebuild):

```powershell
.\web\build.ps1
```

The build needs only `include/`, `src/` and `web/` from the repository. All paths are relative
to the script's own folder. All build output goes to `web\build\` (git-ignored). The first build takes about a minute
because `httplib.h` is large.

If PowerShell blocks scripts, allow them for the current window only:
`Set-ExecutionPolicy -Scope Process Bypass`.

## Test

With the server running (`.\web\run.ps1 -NoBrowser` in another window):

```powershell
.\web\tests\api-smoke.ps1
```

This runs 50 end-to-end checks against the real engine. They cover both industries' completion
paths and exception paths, invalid transitions, dependencies, fork/join, routing, slot refusal,
the approval chain at three different depths, undo, snapshot/restore, instance isolation, and
HTTP errors.

This is an API smoke test for the web layer, not the project's unit-test suite. The spec requires
a real C++ framework (GoogleTest/Catch2/doctest) for that.

## What a marker can do

| Spec requirement | Where in the UI |
|---|---|
| Choose an industry, start independent instances, compare side by side | Industry tab → *Create instance*; *Open* / *Open beside*. After every action an **isolation check** re-reads the other instances from the engine. |
| See the Composite tree, states, assignees, available work | Instance pane: stages → tasks with state colours. *Available work* comes from `DepthFirstIterator`. |
| Take every action; see refusals | Click a task: `assignTo`, `makeAvailable`, `start`, `execute`, `complete`, `approve`, `reject`, `cancel`, `escalate` are always shown. The engine's state decides, and refusals are shown in red. |
| Undo (Command), snapshot/restore (Memento) | *Undo*, *Snapshot*, *Restore* on each instance. |
| Routing, dependency unlocks, contention, chain escalation | Patterns panel, task highlights, *Doctor slots* card, approval-chain card. |
| History / audit trail | *History / audit trail* in each pane, including refused attempts. |
| Which classes and patterns took part | **Patterns in action** panel (right). Each step is tagged *engine*, *industry rule*, *temporary web stand-in* or *not yet integrated*. The engine's own console output is shown too. |
| Generic engine vs industry-specific | *Generic engine vs industry-specific* tab. |
| Reset | *Reset demo* (top right). |

## Layout

```
web/
  build.ps1, run.ps1        build / start (PowerShell)
  README.md, INTEGRATION.md
  server/                   C++ HTTP server + the ONE engine gateway
    main.cpp                routes (cpp-httplib), JSON in/out
    EngineGateway.*         all engine access; mirrors WorkflowManager
    PendingIntegration.*    the single wiring point for Observer + Strategy (Member 6) - TEMPORARY bodies
    WebCommands.*           TEMPORARY command stand-ins (engine CommandProcessor runs them)
    HistoryLog.*            TEMPORARY audit trail
    Trace.h                 "Patterns in action" steps
  scenarios/                industry-specific definitions, rules, data, test adapters
    EducationScenario.*     uses the team's EnrolmentDirector unchanged
    HealthcareDirector.*    second Director for the same DefinitionBuilder
    HealthcareScenario.*
    Scenario.h, RuleHelpers.*
    RecordingDefinitionBuilder.*, NameLookup.*, ParticipantDirectory.*,
    TestConnectors.*, ChainTrace.*    TEMPORARY stand-ins (see INTEGRATION.md)
  temporary_stubs/          TEMPORARY copies of the Connector / ConnectorManager test stubs (Member 7)
  public/                   index.html, app.css, app.js (no frameworks, no build step)
  tests/api-smoke.ps1       end-to-end API checks
  third_party/              vendored header-only libraries
```

Every temporary piece, the member whose fix removes it, and the exact swap steps are in
**[INTEGRATION.md](INTEGRATION.md)**.

## HTTP API

| Method | Path | Body |
|---|---|---|
| GET | `/api/meta` | industries, people, subjects, definitions, chains |
| POST | `/api/reset` | |
| GET | `/api/instances` | summaries |
| POST | `/api/instances` | `{ "industry": "education", "subject": "thandiwe" }` |
| GET | `/api/instances/{id}` | full view (tree, history, snapshots) |
| POST | `/api/instances/{id}/start` · `/undo` · `/snapshot` · `/restore` | |
| POST | `/api/instances/{id}/tasks/{taskId}/{action}` | `assign` needs `{ "participant": "khumalo" }`; `complete` may take `{ "outcome": "urgent" }` |
| GET | `/api/resources/healthcare` | doctor slot Resources as the engine reports them |

Actions: `makeAvailable, assign, start, execute, complete, approve, reject, cancel, escalate`.
A refused action returns `200` with `"ok": false` and a friendly message. Unknown IDs return `404`.

## Third-party code (vendored, MIT licence)

- **cpp-httplib v0.14.3**: Yuji Hirose, https://github.com/yhirose/cpp-httplib.
  `third_party/httplib/httplib.h`, licence in `third_party/httplib/LICENSE`.
- **JSON for Modern C++ (nlohmann/json) v3.11.3**: Niels Lohmann, https://github.com/nlohmann/json.
  `third_party/nlohmann/json.hpp`, licence in `third_party/nlohmann/LICENSE.MIT`.

Both compile with `-std=c++11` on MinGW-w64 g++ 14. The server links `-lws2_32` and is linked
statically, so the `.exe` does not need MSYS2 DLLs at run time.
