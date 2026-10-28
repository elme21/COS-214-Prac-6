# TIVIDY web demo

A locally run website that drives the **real TIVIDY C++11 engine** for two industries:

1. **Education**: university application and admission (the team's `EnrolmentDirector`).
2. **Healthcare**: patient appointment booking at a clinic. It covers urgency routing,
   live competition for limited doctor slots, and a referral approval chain.

The browser contains no workflow logic. Every button calls a small C++ HTTP server (`web/server/`).
The server compiles and links the engine sources in `../src` and `../include` unchanged, and calls
them through one class, `EngineGateway`. This meets spec section 5: a GUI "must still call the C++ engine".

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

`run.ps1` builds if needed, starts the server and opens **http://localhost:8080**. Stop it with **Ctrl+C**.
Options: `-Rebuild`, `-NoBrowser`, `-Port 8081`.

Build only (incremental; `-Clean` for a full rebuild):

```powershell
.\web\build.ps1
```

The build needs only `include/`, `src/` and `web/` from the repository: no stubs and no `tests/`.
All paths are relative to the script's own folder. All output goes to `web\build\` (git-ignored).
The build prints which engine files it skips and why: the Strategy classes do not compile yet, so
`WorkflowManager.cpp` cannot link (see INTEGRATION.md). The first build takes about a minute because
`httplib.h` is large.

If PowerShell blocks scripts, allow them for the current window only:
`Set-ExecutionPolicy -Scope Process Bypass`.

## Test

With the server running (`.\web\run.ps1 -NoBrowser` in another window):

```powershell
.\web\tests\api-smoke.ps1
```

This runs 65 end-to-end checks against the real engine:
- both industries' completion and exception paths;
- invalid transitions, dependencies, fork/join and routing;
- live slot contention, the approval chain at three depths, and failed automated steps;
- real commands and undo, snapshot/restore and isolation;
- the Observer audit trail, the AuditDecorator, and HTTP errors.

This is an API smoke test for the web layer, not the project's unit-test suite. The spec requires a
real C++ framework (GoogleTest/Catch2/doctest) for that.

## What a marker can do

| Spec requirement | Where in the UI |
|---|---|
| Choose an industry, start independent instances, compare side by side | Industry tab → *Create instance*; *Open* / *Open beside*. After every action an **isolation check** re-reads the other instances from the engine. |
| See the Composite tree, states, assignees, available work | Instance pane: stages → tasks with state colours. *Available work* is the engine's `ReadyTaskIterator`. |
| Take every action; see refusals | Click a task. `assignTo`, `makeAvailable`, `start`, `execute`, `complete`, `approve`, `reject`, `cancel` and `escalate` are always shown. Each runs the real command; refusals show in red. |
| Undo (Command), snapshot/restore (Memento) | *Undo* (`CommandProcessor::undo`), *Snapshot*, *Restore* on each instance. |
| Routing, dependency unlocks, contention, chain escalation | The Patterns panel, task highlights, the *Doctor slots* card (live places) and the approval-chain card. |
| History / audit trail | *Audit trail*: the real `AuditLogger`, `CommandProcessor::getHistory()` and `NotificationSender`. |
| Which classes and patterns took part | **Patterns in action** panel (right). Each step is tagged *engine*, *industry rule*, *temporary web stand-in* or *not yet integrated*, and the engine's console output is shown too. |
| Generic engine vs industry-specific | The *Generic engine vs industry-specific* tab, with the pattern status for this build. |
| Reset | *Reset demo* (top right). |

## Layout

```
web/
  build.ps1, run.ps1        build / start (PowerShell)
  README.md, INTEGRATION.md
  server/                   C++ HTTP server + the ONE engine gateway
    main.cpp                routes (cpp-httplib), JSON in/out
    EngineGateway.*         all engine access; mirrors WorkflowManager
    PendingIntegration.*    TEMPORARY: Strategy hook (manual assignment until Strategy compiles)
    Trace.h                 "Patterns in action" steps
  scenarios/                industry-specific definitions, rules and data
    EducationScenario.*     runs the team's EnrolmentDirector on the real builder
    HealthcareDirector.*    second Director for the same DefinitionBuilder
    HealthcareScenario.*
    Scenario.h, RuleHelpers.*, Staff.*     industry interface, rule helpers, seeded people
    TaskLookup.*, StageNameRecorder.*, ChainTrace.*    TEMPORARY stand-ins (see INTEGRATION.md)
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
| GET | `/api/instances/{id}` | full view (tree, audit trail, command history, snapshots) |
| POST | `/api/instances/{id}/start` · `/undo` · `/snapshot` · `/restore` | |
| POST | `/api/instances/{id}/tasks/{taskId}/{action}` | `assign` needs `{ "participant": "khumalo" }`; `complete` may take `{ "outcome": "urgent" }` |
| GET | `/api/resources/healthcare` | doctor slot Resources: capacity, free places, holders |

Actions: `makeAvailable, assign, start, execute, complete, approve, reject, cancel, escalate`.
A refused action returns `200` with `"ok": false` and a friendly message. Unknown IDs return `404`.

## 10-minute demo script

Press **Reset demo** first. Keep the *Patterns in action* panel visible: after each click, point at
the green (engine) steps.

**Education (about 5 minutes)**
1. **0:00** Create two instances for *Thandiwe*, then use *Open* and *Open beside*. Press **Start** on
   EDU-1: *Available work* comes from `ReadyTaskIterator`. Click **complete** on *Check Documents*: it is
   refused by its state (red). Click **makeAvailable**: it is refused by the `ResourceCoordinator`
   (dependency not met).
2. **1:00** On *Submit Application*, run assignTo (Applicant), start, complete. The panel shows
   `CompleteCommand` → `CommandProcessor`, then `ITEM_COMPLETED` on the `WorkflowEventBus`. The
   **DependencyActivator** unlocks *Check Documents*. Do the same for *Check Documents* (Ms Khumalo):
   the fork unlocks both verifications.
3. **2:00** On *Verify Personal Details*, run assign (TIVIDY automation), start, **execute**: the
   Template Method completes it, and the join still waits. Do the same for *Verify Results*, and the
   join opens *Faculty Checks*. Complete it (Dr Botha).
4. **3:00** *Approve or Reject Application* carries an **AuditDecorator** tag. Assign, start (point at the
   audit line), then **execute**: risk 15 stops at the Faculty reviewer. Notify Applicant. The status
   becomes Finished, and the isolation line shows EDU-2 unchanged. Open the *Audit trail*: these are
   the `AuditLogger` lines plus `CommandProcessor::getHistory()`.
5. **4:00** Exception paths.
   - Create *Aisha* (Cambridge): her *Verify Results* is marked "automated step will fail".
     **execute** leaves it InProgress (amber); then **escalate**, assign Ms Khumalo, start, complete.
   - Optionally, create *Sipho*: the decision travels Faculty reviewer → HOD → Registrar, which rejects,
     and Notify Applicant still unlocks.
   - Show **Undo** (one command at a time) and **Snapshot / Restore**.

**Healthcare (about 5 minutes)**
6. **5:00** Create *Mandla* and *Bongani* side by side and start both. For each: Register (Nandi), Verify
   Medical Aid (execute), then Triage complete with outcome **routine**. The routing rule cancels the
   emergency branch with real `CancelCommand`s.
7. **6:30** **Live contention.** Assign Mandla's *Book Appointment Slot* to **Dr van der Merwe**: the slot
   is granted, and the *Doctor slots* card shows 0 of 1 places free. Assign Bongani's booking to the
   same doctor: it is **refused** (amber). **start** is refused too. **escalate**, then assign
   **Dr Mokoena**: granted.
8. **7:30** Mandla: start and complete the booking, then **cancel** *GP Consultation* (no-show). The
   follow-ups are cancelled and Dr van der Merwe's place is released (card shows 1 of 1 free).
   **Undo** once: only the last `CancelCommand` is reverted.
9. **8:30** Bongani: complete the booking and the GP consultation, then **execute** *Specialist
   Referral*: R9 800 travels Nurse → GP → Specialist, who approves. Then create *Lerato*: triage
   **urgent** cancels the booking branch, and she goes straight to the doctor on duty.
10. **9:30** *Generic engine vs industry-specific* tab: the classes both industries share, the pattern
    status (Strategy and Facade honestly marked), and the remaining stand-ins.

## Third-party code (vendored, MIT licence)

- **cpp-httplib v0.14.3**: Yuji Hirose, https://github.com/yhirose/cpp-httplib.
  `third_party/httplib/httplib.h`, licence in `third_party/httplib/LICENSE`.
- **JSON for Modern C++ (nlohmann/json) v3.11.3**: Niels Lohmann, https://github.com/nlohmann/json.
  `third_party/nlohmann/json.hpp`, licence in `third_party/nlohmann/LICENSE.MIT`.

Both compile with `-std=c++11` on MinGW-w64 g++ 14. The server links `-lws2_32` and is linked
statically, so the `.exe` does not need MSYS2 DLLs at run time.
