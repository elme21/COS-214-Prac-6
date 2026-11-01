# TIVIDY web demo

A locally run website that drives the **real TIVIDY C++11 engine** for **any** workflow:

- Two **templates** ship in `web/templates/`:
  - **University admission** (Education): document loop, fork/join, an approval chain.
  - **Patient booking** (Healthcare): urgency routing, live competition for doctor slots, a referral chain.
- **+ New workflow** opens a six-step wizard (Basics, People, Steps, Approvals, Decisions, Review) in
  plain language, with each engine name in small grey text. Start from scratch or from a template,
  change anything, and create a new industry live. It gets its own tab and works exactly like the
  templates. Problems are listed per step and on Review, each with a "Go to step" link.

The browser contains no workflow logic and no industry. Every scenario is a JSON **spec**. One
`GenericDirector` (web/scenarios) builds any spec with the engine's real `CodeDefinitionBuilder` and
registers it in the real `DefinitionRegistry`. Every button calls a small C++ HTTP server, which calls
the engine through one class, `EngineGateway`.

Everything runs **offline**: no CDNs, fonts or downloads. All state is in memory: restarting the server
(or **Reset demo**) removes custom scenarios and reloads the templates.

## Dependencies

- Windows with **Windows PowerShell 5.1**.
- **MSYS2 UCRT64 g++** (tested with g++ 14.2.0) on `PATH`, e.g. `C:\msys64\ucrt64\bin`.
  Build from PowerShell, not Git Bash.
- A browser.
- Nothing to download: the two header-only libraries the server uses are vendored in `web/third_party/`
  (see the end of this file).

## Build and run

```powershell
.\web\run.ps1
```

`run.ps1` builds if needed, starts the server and opens **http://localhost:8080**. Stop it with **Ctrl+C**.
Options: `-Rebuild`, `-NoBrowser`, `-Port 8081`.

```powershell
.\web\build.ps1
```

`build.ps1` builds only (incremental; add `-Clean` for a full rebuild). It needs only `include/`, `src/`
and `web/`. All paths are relative to the script's folder, and the output goes to `web\build\`
(git-ignored). Every engine file compiles with `-std=c++11 -Wall -Wextra`, and the build has zero
warnings; any compiler message is printed. The first build takes about a minute (`httplib.h` is large).

If PowerShell blocks scripts: `Set-ExecutionPolicy -Scope Process Bypass`.

## Test

With the server running (`.\web\run.ps1 -NoBrowser` in another window):

```powershell
.\web\tests\api-smoke.ps1
```

71 end-to-end checks against the real engine. They cover:
- **Scenarios:** listing templates; building both as scenarios; a brand-new "Car insurance claim"
  created through the API.
- **Validation:** rejecting invalid specs (missing fields, unknown references, dependency cycles,
  empty chains, bad input).
- **Decisions and routing:** a rule-based decision, a manual override, and runtime-chosen decisions.
- **Approval chains:** escalation through generic levels.
- **Engine behaviour:** live resource contention, automated-step failures, real commands and undo,
  snapshots, isolation, auto-assignment with both strategies, and the Observer audit trail.

It is an API smoke test for the web layer, not the project's unit-test suite.

## Adding a scenario without code

Drop a `.json` file in `web/templates/` and restart the server: it appears as a tab. Or use
**+ New workflow** in the browser (memory only; *Advanced: View as data (JSON)* on its Review step
shows the spec it sends). The format:

```jsonc
{
  "name": "Car insurance claim",            // unique; becomes the definition name in the DefinitionRegistry
  "industry": "Insurance",                  // label only
  "description": "...", "idPrefix": "CLM",  // instance ids CLM-1, CLM-2, ...
  "subjectField": "claimant",               // input shown as an instance's title
  "inputs": [ { "key": "claimValue", "label": "Claim value (R)", "type": "number" },          // text | number | choice
              { "key": "policyActive", "label": "Policy active", "type": "choice", "options": ["yes", "no"] } ],
  "participants": [ { "id": "manager", "name": "Max van Rooyen", "role": "Claims manager" } ],
  "resources": [ { "id": "bay1", "name": "Repair bay 1", "capacity": 1, "owner": "" } ],   // owner: optional participant id
  "stages": [ { "id": "Intake", "name": "Intake",
      "tasks": [ { "id": "LodgeClaim", "name": "Lodge claim", "type": "manual",            // manual | automated | approval
                   "role": "Claims clerk", "dependsOn": [], "strategy": "role",          // none | role | workload
                   "resource": "bay1",                                                   // or "@assignee" (the assignee's own resource)
                   "decorators": ["audit"],                                              // what addDecorator supports
                   "onComplete": { "outcome": "Lodged R{claimValue}", "set": {}, "cancel": [], "unlock": [], "release": [] },
                   "onReject": {}, "onCancel": {} } ],
      "stages": [ /* nested sub-stages, same shape */ ] } ],
  // automated task:  "succeeds": true, "successRule": { "field": "policyActive", "op": "==", "value": "yes" }
  // approval task:   "requestValue": { "field": "claimValue" } (or { "value": 450 }),
  //                  "chain": [ { "label": "Approver level 1", "role": "Claims manager", "limit": 100000 },
  //                             { "label": "Final approver",   "role": "Finance director", "limit": 250000 } ]
  "decisions": [ { "id": "route", "label": "Claim route", "after": "AssessDamage", "mode": "rule",   // rule | manual
                   "branches": [ { "id": "fast", "label": "Fast track", "tasks": ["FastPayout"] },
                                 { "id": "full", "label": "Full review", "tasks": ["ApproveClaim", "Payout"] } ],
                   "rules": [ { "field": "claimValue", "op": ">", "value": "50000", "branch": "full" } ],
                   "default": "fast" } ],
  "seeds": [ { "label": "Nomvula Khumalo", "summary": "...", "data": { "claimValue": 180000, "policyActive": "yes" } } ]
}
```

How the spec maps to the engine:
- **Dependencies** are AND-only (the engine's Mediator). When a decision's `after` task completes, the
  chosen branch stays and every other branch's tasks are cancelled with real `CancelCommand`s.
- A **rule** decision picks its branch from input data; the user may override it when completing.
  A **manual** decision is chosen at runtime, with the rules only suggesting.
- **Effects** run with real engine calls and do not cascade.
- A task with a **resource** only starts once the `ResourceCoordinator` granted it.

The server validates every spec first, reporting each problem with its location: missing fields,
duplicate or unknown ids, unknown roles, fields, resources or tasks, dependency cycles, empty
approval chains, and decisions with fewer than two branches.

## What a marker can do

| Spec requirement | Where in the UI |
|---|---|
| Choose an industry, start independent instances, compare side by side | One tab per scenario. Fill in the information for the case (or pick an example case), then *Create case*; use *Open* / *Open beside*. After every action an isolation check re-reads the other instances from the engine. |
| Composite tree, states, assignees, available work | Instance pane: stages, sub-stages and tasks with state colours. *Available work* is `ReadyTaskIterator`. |
| Every action; refusals | Click a task: `assignTo`, *auto-assign (strategy)*, `makeAvailable`, `start`, `execute`, `complete`, `approve`, `reject`, `cancel`, `escalate`. Each runs the real command or engine call; refusals show in red, warnings in amber. |
| Routing / decisions | A task with a *decision* tag shows a branch selector when completed: the rule's choice or the runtime choice. |
| Undo, snapshots | *Undo* (`CommandProcessor::undo`), *Snapshot*, *Restore*. |
| Contention, chain escalation | *Resources* card (live places) and *Approval chain* cards with generic levels. |
| History / audit trail | `AuditLogger` lines, `CommandProcessor::getHistory()`, `NotificationSender`. |
| Which classes and patterns took part | **Patterns in action** panel; every step is tagged *engine*, *scenario rule*, *temporary web stand-in* or *not used*. |
| Generic engine vs scenario-specific | *Generic engine* tab: the engine classes every scenario reuses, and each scenario's spec. |
| Reset | *Reset demo* removes custom scenarios and reloads the templates. |

## Layout

```
web/
  build.ps1, run.ps1, README.md, INTEGRATION.md
  templates/                 scenario specs loaded at startup (one .json per scenario)
  server/                    C++ HTTP server + the ONE engine gateway
    main.cpp                 routes (cpp-httplib), JSON in/out
    EngineGateway.*          all engine access; generic; mirrors WorkflowManager
    Trace.h                  "Patterns in action" steps
  scenarios/                 generic scenario support (no industry code)
    ScenarioSpec.*           spec model, parser and validator
    GenericDirector.*        the one Director for the real CodeDefinitionBuilder
    ScenarioRuntime.*        owns a scenario's Participants, Resources, approval chains and strategies
    Rules.*                  evaluates rules on input data
    TaskLookup.*, StageNames.*  TEMPORARY stand-ins (see INTEGRATION.md)
  public/                    index.html, app.css, app.js (no frameworks, no build step)
  tests/api-smoke.ps1        end-to-end API checks
  third_party/               vendored header-only libraries
```

## HTTP API

| Method | Path | Body / notes |
|---|---|---|
| GET | `/api/templates` | template specs (for the wizard's "Use a template") |
| GET | `/api/scenarios` | every scenario: inputs, people, resources, chains, decisions, seeds, definition tree, spec |
| POST | `/api/scenarios/validate` | a spec, returns `{ok, errors[]}` |
| POST | `/api/scenarios` | a spec, builds and registers it (400 with `errors[]` if invalid) |
| POST | `/api/reset` | removes custom scenarios, reloads templates |
| GET / POST | `/api/instances` | list / `{ "scenario": "car-insurance-claim", "inputs": { ... } }` |
| GET | `/api/instances/{id}` | full view |
| POST | `/api/instances/{id}/start` · `/undo` · `/snapshot` · `/restore` | |
| POST | `/api/instances/{id}/tasks/{taskId}/{action}` | `assign` needs `{ "participant": "manager" }`; `complete` may take `{ "branch": "full" }` |
| GET | `/api/resources/{scenario}` | resources: capacity, free places, holders |

Actions: `makeAvailable, assign, autoAssign, start, execute, complete, approve, reject, cancel, escalate`.

Errors always come back as JSON `{ "ok": false, "message": "...", "errors": [...] }`: 400 for a body
that is not valid JSON or not a JSON object, an invalid spec or an unknown action; 404 for an unknown
scenario, instance, task or endpoint. A refused engine action is not an error: it returns 200 with
`"ok": false` and the engine's reason.

## 10-minute demo script

Press **Reset demo** first. Keep *Patterns in action* visible and point at the green (engine) steps.

**University admission (about 3.5 minutes)**
1. **0:00** Tab *University admission*. Example case **Thandiwe**: *Create case* twice, then *Open beside*.
   **Start** EDU-1.
   - Click **complete** on *Check Documents*: refused by its state.
   - Click **makeAvailable**: refused by the Mediator.
2. **0:45** *Submit Application*: **auto-assign** (`TaskRouter` + `RoleBasedAssignment`), then start
   and complete. `ITEM_COMPLETED` is published and **DependencyActivator** unlocks *Check Documents*.
   Then *Check Documents*: auto-assign (`WorkloadAssignment`), start, complete. The fork opens both
   verifications.
3. **1:30** Both verifications: auto-assign, start, **execute** (Template Method). The join then opens
   *Faculty Checks*: complete it.
4. **2:15** *Approve or Reject Application* (nested sub-stage *Admission decision*, **AuditDecorator**
   tag): assign, start (point at the audit line), **execute**. Risk 15 stops at **Approver level 1**.
   Notify: the instance is Finished and EDU-2 is unchanged (isolation line). Open the audit trail.
5. **3:00** Exception: example case **Aisha** (Cambridge). *Verify Results* shows "automated step will
   fail". **execute** leaves it InProgress (amber), then escalate, reassign and complete by hand. Show
   **Undo** (one command) and **Snapshot / Restore** (status, case data and tree come back).

**Patient booking (about 3.5 minutes)**
6. **3:30** Tab *Patient booking*: example cases **Johan** and **Mandla**, side by side. For each: Register,
   Verify Medical Aid (execute), Triage. On completing Triage, the **Triage outcome** selector is a
   runtime decision: choose **Routine**, and the emergency branch is cancelled with real commands.
7. **4:45** **Live contention.** Assign Johan's *Book Appointment Slot* to **Dr van der Merwe**: granted,
   and the Resources card shows 0 of 1 free. Assign Mandla's to the same doctor: **refused** (amber).
   **start** is refused too. Escalate, then reassign to **Dr Mokoena**: granted.
8. **5:45** Johan: complete the booking, reminder and GP consultation (the slot is released). Then
   **execute** the referral: R450 stops at **Approver level 1**. Mandla: **cancel** the GP consultation
   (no-show): follow-ups are cancelled and the slot is released.
9. **6:30** Example case **Lerato**: at Triage the suggestion is **Urgent**. Complete, and the booking branch is
   cancelled; she goes straight to the doctor on duty.

**A new industry, live (about 2 minutes)**
10. **7:00** **+ New workflow**, **Start from scratch**.
    - **1 Basics:** name *Car insurance claim*, industry *Insurance*; information collected: Claimant
      (Text), Claim value (Number), Policy active (Pick from a list: yes, no).
    - **2 People:** clerk, assessor, claims manager, finance director, system.
    - **3 Steps:** phase *Intake* (Lodge claim, then Assess damage); phase *Settlement* with Fast-track
      payout (Happens automatically, only succeeds if Policy active equals yes), Approve claim (Needs
      approval) and Pay out (Happens automatically, after Approve claim).
    - **4 Approvals:** Approve claim takes its amount from Claim value; first approver up to 100 000,
      final approver up to 250 000.
    - **5 Decisions:** split point after Assess damage, chosen automatically: "If Claim value is more
      than 50000, take path Full review"; otherwise Fast track.
    - **6 Review:** read the summary, then **Create workflow**: a new tab appears.
11. **9:00** Create R180 000: the rule routes it to full review, Approver level 1 passes it to the Final
    approver, who approves, and Pay out finishes. Create R12 000 with a lapsed policy: fast track, and
    the payout fails (amber).
12. **9:40** *Generic engine* tab: the same engine classes run all three industries; only the spec
    differs.

(To save time in the demo, start from the *Car insurance claim* JSON: keep a copy as
`web/templates/car-insurance-claim.json`, or fill the wizard while talking.)

## Third-party code (vendored, MIT licence)

- **cpp-httplib v0.14.3**: Yuji Hirose, https://github.com/yhirose/cpp-httplib.
  Licence in `third_party/httplib/LICENSE`.
- **JSON for Modern C++ (nlohmann/json) v3.11.3**: Niels Lohmann, https://github.com/nlohmann/json.
  Licence in `third_party/nlohmann/LICENSE.MIT`.

Both compile with `-std=c++11` on MinGW-w64 g++ 14. The server links `-lws2_32` and is linked
statically, so the `.exe` needs no MSYS2 DLLs at run time.
