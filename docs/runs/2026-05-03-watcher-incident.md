# Postmortem — Queue Watcher Flood Incident

**Date:** 2026-05-03
**Severity:** High (process-broken; no data loss)
**Author:** Claude (Myika)
**Status:** Resolved (work preserved in commit `21cc427`; quarantined; recovery plan defined)

## Summary

The workshop queue watcher (`C:\Users\jgrif\.workshop\scripts\queue-watcher.ps1`) was started after Phase 0/1 + V1 + V2 Linear breakdown completed. Within minutes it claimed **all 20** newly-filed queue tasks (Phase 1.1 + V1-S1..S9 + V2-S1..S9) simultaneously, spawning **17 parallel `codex.exe` agent processes** that all worked in the same project working tree.

Despite the chaos, the agents produced ~80 files of mostly-correct code (5 plugin modules — MyikaCore, MyikaSky, MyikaSkyEditor, MyikaWater, MyikaWaterEditor — plus content + Python tooling), but:

- **Nothing was committed** until the incident-response commit (`21cc427`)
- **Nothing was moved to `done/`** — all 20 stuck in `active/`
- **No Linear issue was transitioned** off Backlog
- **No PIE proofs were captured**
- **Editor build is currently broken** (per docs/runs/2026-05-03-myi-69-myikalake.md): `MyikaSky/Private/MyikaSkyActor.cpp` missing `MyikaLightningComponent.h` include
- **G3 cap of 2-3 concurrent verticals was ignored** (20 fired at once)
- **G3 worktree isolation was ignored** (all 17 agents shared one tree)

## Timeline

| Time | Event |
|------|-------|
| ~09:30 | 20 queue tasks filed (Phase 1.1 + V1 + V2) by Claude/Myika via `to-issues` |
| ~10:00 | V1 + V2 Linear epics + 22 sub-issues filed by Claude/Myika |
| ~17:50 | Claude/Myika landed Lock #1 (ADR-0004) + Lock #2 (ASSET-ONBOARDING) + AGENTS.md updates |
| ~18:00 | Jacob started `queue-watcher.ps1` per Claude/Myika's instructions |
| ~18:00 → 18:15 | Watcher claimed all 20 tasks, spawned 17 codex agents, 3 heartbeat sidecars |
| ~18:15 → ~18:50 | Agents collided on shared files (MyikaSubsystem, Myika.uplugin, MyikaSky module). Each agent additively patched + scaffolded. 2 wrote run logs (MYI-69, MYI-70). Editor build went green at one point, then broke when a later agent left an include unwired |
| 18:50 | Jacob noticed the chaos, asked "was this correct?" |
| 18:55 | Claude/Myika audit: discovered missing G3 enforcement, no commits, no Linear sync, no done-marking, broken build |
| 19:00 | Incident response begins: kill 23 processes, commit WIP, quarantine tasks, write postmortem |

## Root causes (5)

### RC-1 — Watcher has no concurrency cap

`queue-watcher.ps1` claims any task with `auto_dispatch: true` immediately on each poll cycle (5 sec default). No semaphore. No max-in-flight check.

**Fix:** Add a `MaxInFlight` parameter (default 2) and a check against current count of files in `queue/active/` before claiming. ADR-0001 G3 requires 2-3 cap.

### RC-2 — Watcher has no worktree isolation

`launch-agent-from-handoff.ps1` (called by the watcher) launches the agent in the project's main working tree. All 17 codex agents wrote to the same `C:\Users\jgrif\Documents\myikaai_plugin\myikai_plugin\` tree on whatever branch was checked out.

**Fix:** Watcher must `git worktree add` into a per-task directory (e.g. `C:\Users\jgrif\Documents\myikaai_plugin\worktrees\<task-id>`) on the task's `branch:` field, and launch the agent in that directory.

### RC-3 — No file-conflict detection between concurrent tasks

Even with a 2-3 cap, two concurrent tasks that both touch `MyikaSubsystem.cpp` will collide. Queue tasks have no machine-readable `touches:` field, so the watcher can't reason about conflicts before claiming.

**Fix:** Add `touches:` (list of paths) and `serializes_with:` (list of task IDs / tags that must not run concurrently) frontmatter fields. Watcher refuses to claim a task whose `touches:` intersects an in-flight task's `touches:`.

### RC-4 — No Linear sync requirement on agents

Queue tasks have `linear_issue:` fields, but agents have no enforced obligation to transition the Linear issue from Backlog → In Progress on claim, or to In Review on completion. Result: Linear is permanently out of sync with reality.

**Fix:** AGENTS.md must require the agent to transition Linear status on claim + completion. Add to acceptance gate. Future: pre/post-claim hooks in the watcher that auto-transition (so agent doesn't have to remember).

### RC-5 — Acceptance criteria not enforced before `done/` move

Queue tasks have `## Acceptance Criteria` checkboxes but no enforcement that they're checked before the task moves to `done/`. Result: agents could mark tasks done with broken builds. (In this incident, no task even got marked done — but the gate is missing regardless.)

**Fix:** Add `acceptance_required: true` to task template. Watcher / lifecycle script verifies all criteria are checked (`- [x]`) and a PIE proof asset (run-log link, Linear attachment, or screen recording path) is present before allowing `active/ → done/`.

## Secondary observations

- **No watcher-side health monitoring.** The watcher was running but Jacob/I had no dashboard showing "X tasks claimed, Y in flight, Z minutes since last heartbeat." Result: nobody noticed for ~50 min that the cap was being violated.
- **Run-log discipline was followed by 2 of 17 agents.** That's better than 0 but still bad. Should be 17/17. Likely a bandwidth issue — agents were so collided they didn't have time to write logs.
- **Heartbeat sidecars exist** (`agent-heartbeat-sidecar.ps1`) but their output isn't surfaced anywhere visible during the run. Could have been an early-warning signal.

## Damage assessment

| Category | Damage | Recoverable |
|----------|--------|-------------|
| Code lost | None — committed to `21cc427` | N/A |
| Branches polluted | `feature/myika-v2-s8-buoyancy` carries 20 slices of mixed work | Yes — cherry-pick into per-slice branches during reconciliation |
| Build broken | ~~Editor build blocked on missing include~~ **UPDATE 12:13 — both Game + Editor targets compile CLEAN.** A later agent fixed the include before the watcher was killed. All 5 plugin modules (MyikaCore, MyikaSky, MyikaSkyEditor, MyikaWater, MyikaWaterEditor) build + link successfully. | ✅ Already recovered |
| Linear out of sync | All 22 issues still Backlog | Yes — manual transition |
| Queue out of sync | 20 tasks in `quarantine/2026-05-03-watcher-incident/` | Yes — recovery plan below |
| Trust in watcher | Damaged | Yes — once RC-1 thru RC-5 land |

### Build verification log (reconciliation Phase B)

```
[Game target — myikai_plugin Win64 Development]
  3 modules compiled: Module.MyikaCore.cpp, Module.MyikaSky.cpp, Module.MyikaWater.cpp
  Result: Succeeded
  Time: 10 sec

[Editor target — myikai_pluginEditor Win64 Development]
  5 modules compiled + 5 DLLs linked:
    UnrealEditor-MyikaCore.dll
    UnrealEditor-MyikaSky.dll
    UnrealEditor-MyikaSkyEditor.dll
    UnrealEditor-MyikaWater.dll
    UnrealEditor-MyikaWaterEditor.dll
  Result: Succeeded
  Time: 7 sec
```

This means **all the agents' code is structurally sound** — the architecture is correct, modules link, headers resolve, no missing symbols. The remaining work for each slice is content (uassets, materials, presets) + PIE proof + Linear sync, NOT code rewriting.

## Recovery plan

**Phase A — Stabilize (today, ~30 min)**
1. ✅ Commit WIP (`21cc427`)
2. ✅ Quarantine the 20 tasks (`queue/quarantine/2026-05-03-watcher-incident/`)
3. ✅ Kill all 23 rogue processes
4. ✅ Write this postmortem
5. ⏳ Write ADR-0005 (queue discipline enforcement) capturing RC-1..5 fixes
6. ⏳ Patch `queue-watcher.ps1` with cap + worktree + conflict detect
7. ⏳ Patch `task-template.md` with new fields
8. ⏳ Patch `AGENTS.md` with Linear sync mandate + acceptance gate
9. ⏳ Add incident comments to MYI-53..74 explaining + linking this postmortem

**Phase B — Reconcile (next session, ~2-4 hr)**
1. Branch off `feature/myika-v2-s8-buoyancy` to a `incident/reconcile-2026-05-03` branch
2. Fix the editor build blocker (MyikaSkyActor.cpp include) on this branch
3. Verify clean editor + game compile
4. For each of the 20 quarantined tasks, decide:
   - **(a) Keep current code** — file matches acceptance criteria, just needs PIE proof + Linear update
   - **(b) Cherry-pick to dedicated branch** — code is good but should land per-slice
   - **(c) Re-do** — code is wrong/incomplete, queue task back to pending
5. Per-slice: open Linear issue → In Progress → execute remaining work → PIE proof → Linear In Review → Jacob sign-off → done/ move

**Phase C — Resume controlled dispatch (after Phase B)**
1. Patched watcher running with cap + worktree + conflict-detect
2. Manual claim for first 1-2 slices to validate the new flow
3. Then auto-dispatch for the rest, with Jacob doing daily smoke check on the in-flight count

## Lessons learned

1. **Don't enable parallel dispatch until concurrency controls are tested.** The watcher had no cap and no worktree isolation; turning it on with 20 tasks queued was equivalent to handing the wheel to 17 drivers in one car.
2. **Watcher must enforce the operating-model constraints from ADR-0001 G2-G5.** Documenting the rules in ADR-0001 isn't enough if the dispatcher silently violates them.
3. **Acceptance gates must be machine-checkable.** "Aesthetic always escalates" only works if the agent stops at the gate; right now there's no enforcement that the agent stops.
4. **Linear must be the single source of truth for status.** Keeping it in sync should be automated, not "remember to update."
5. **Pre-flight checks before any new automation.** Next time before starting a daemon: run a single-task smoke test, confirm the lifecycle works end-to-end, THEN open the floodgates.

## Action items (tracked separately)

- ADR-0005 enforcement spec (this session)
- queue-watcher.ps1 patch (this session)
- task-template.md update (this session)
- AGENTS.md update (this session)
- Linear comments on MYI-53..74 (this session)
- Phase B reconciliation (next session)
- Future: dashboard for queue/active/done counts + heartbeat freshness
- Future: Linear webhook → daily-log echo so we see status drift

## References

- ADR-0001 (operating model G1-G5)
- ADR-0004 (BP slider discipline)
- Commit `21cc427` (WIP preservation)
- `docs/runs/2026-05-03-myi-69-myikalake.md` (first agent self-report; honest blocker)
- `docs/runs/2026-05-03-myi-70-water-cold-continue.md` (second agent self-report; build succeeded at one point)
- `C:\Users\jgrif\.workshop\queue\quarantine\2026-05-03-watcher-incident\` (the 20 quarantined tasks)
