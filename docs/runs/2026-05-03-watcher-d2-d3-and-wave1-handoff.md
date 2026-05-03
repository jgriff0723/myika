# 2026-05-03 — Watcher D2/D3 patches + Wave 1 PIE handoff

**Author:** Claude (Myika)
**Branch:** `incident/reconcile-2026-05-03`
**Trigger:** Jacob said "fix it and move forward, optimized — use what we have."

## Summary

Closed two of the highest-risk ADR-0005 enforcement gaps (D2 + D3) in the
workshop watcher, drained the queue of 13 superseded MyikaWave tasks that
were silently blocking auto-dispatch, and promoted the 3 reconciliation-
clean Wave 1 slices into a new `awaiting-jacob-pie/` lane so they're not
falsely marked done before Jacob walks PIE.

## What landed (workshop infra side)

### ADR-0005 D2 — per-task git worktree isolation (`launch-agent-from-handoff.ps1`)

Added `Ensure-WorktreeForTask` helper. When `-ClaimNext` claims a task,
the launcher now:

1. Reads `branch:` from task frontmatter (REQUIRED — fails loud if absent)
2. Computes worktree path: `<repo>/../worktrees/<task-id>`
3. If worktree exists: verifies it's on the expected branch, fails loud if drift
4. If branch already exists in repo: `git worktree add <path> <branch>`
5. If branch is new: `git worktree add -b <branch> <path>`
6. Routes the agent into the worktree, NOT the shared repo trunk

This is the structural fix for the 2026-05-03 incident's RC-2 (17 agents
sharing one tree). Smoke-tested via `-PrintOnly`.

### ADR-0005 D3 — watcher-side touches/serializes_with conflict detection (`queue-watcher.ps1`)

Added `ConvertTo-FrontmatterList` (parses `[a, b]` / `a, b` / `[]` style)
and `Test-TaskConflict` (intersects candidate `touches:` and
`serializes_with:` against every in-flight task in `active/`).

Wired into `Get-EligibleTask`: any candidate whose `touches:` overlaps
an in-flight task, OR shares a `serializes_with:` key, is skipped with
a `Skip <name>: <reason>` log line. Smoke-tested via `-Once -DryRun`.

This is the structural fix for RC-3 (no file-level conflict reasoning).

### Queue triage — superseded MyikaWave + watcher-incident promotions

Quarantined 12 stale tasks into `queue/quarantine/2026-05-03-superseded-myikawave/`
(README explains: these targeted the abandoned `MyikaAI/myika_plugin`
repo and were superseded by the current `myikai_plugin` Myika plugin
work). 5 had been stuck in `active/` since 2026-05-01, 7 in `pending/`
since 2026-05-01. They were silently keeping the watcher's in-flight
counter wrong.

Created `queue/awaiting-jacob-pie/` lane with a README documenting the
discipline: build-validated tasks live here until Jacob walks PIE,
flips remaining acceptance checkboxes, and moves them to `done/`. The
watcher does NOT poll this folder, so cap math stays clean.

Promoted the 3 reconciliation-clean Wave 1 slices into the new lane:

| Task | Linear | What's verified | What needs Jacob |
|------|--------|-----------------|------------------|
| Phase 1.1 — MPC_Myika asset | MYI-54 | MPC + M_MPCDebug exist on disk, build clean | PIE walk: scrub MPC values, debug plane reacts |
| V1-S1 — MyikaSky scaffold | MYI-55 | Module + actor + L_SkyTest map exist, link clean | PIE walk: sky renders, AMyikaSkyActor in Place Actors panel |
| V2-S1 — MyikaWater scaffold | MYI-66 | Module + controller + L_WaterTest map exist, link clean | PIE walk: ocean renders, AMyikaWaterController in Place Actors panel |

Each updated task carries: `auto_dispatch: false`, `touches:`,
`serializes_with:`, the new acceptance fields, and a `<!-- ADR-0005
reconciliation note -->` comment block explaining provenance + outstanding
work. Build acceptance criteria flipped to `[x]` with citations to the
verifying commit (`f691bae`); PIE/editor criteria left `[ ]` with
`*(NEEDS JACOB ...)*` annotations.

## What's still pending (ADR-0005 follow-ups, not blocking forward motion)

These remain TODO but the watcher can now be safely re-enabled:

- **D4** — Linear sync within 30 sec of claim. Currently agent-side
  (documented in AGENTS.md). Watcher-side D9 patch is the proper fix.
- **D5** — lifecycle script verifies acceptance checkboxes + `pie_proof:`
  before allowing `active/ → done/`. Currently honor-system (the
  `awaiting-jacob-pie/` lane is a lighter manual workaround).
- **D6** — heartbeat-stale auto-kill of agents that go silent.
- **D9** — watcher-side Linear auto-transition (Backlog→In Progress on
  claim, →In Review on completion). Defers MCP-from-PowerShell complexity.
- **D10** — agent-bootstrap injection of mandatory pre-implementation
  reads (currently in AGENTS.md, but not yet enforced in the launcher prompt).

## Watcher state after this turn

```json
{
    "watcher_pid":  37176,
    "max_in_flight":  2,
    "effective_cap":  1,
    "probe_mode":  true,
    "in_flight":  1,
    "pending_count":  1
}
```

`in_flight: 1` is the leftover MYI-30 server-telemetry task (left
intentionally — different concern from Myika plugin). `pending_count: 1`
is the leftover openclaw stabilize task. Both pre-incident and intentionally
left for Jacob to triage.

## Reconciliation queue status (overall)

- 17 quarantined Myika tasks remaining in `queue/quarantine/2026-05-03-watcher-incident/`
  (Wave 2-5 reconciliation, ~13-20hr)
- 3 Myika tasks in `queue/awaiting-jacob-pie/` (Wave 1, ~30 min PIE walks each)
- 12 superseded MyikaWave tasks quarantined (no further action expected)

Reconciliation triage rubric is in `docs/runs/2026-05-03-reconciliation-triage.md`.

## Recommended Jacob next-actions (when convenient)

1. **30 min** — Open editor, walk PIE on `L_SkyTest` and `L_WaterTest`,
   inspect MPC params, snapshot proofs, move 3 awaiting tasks to `done/`,
   flip Linear MYI-54/55/66 to In Review with attachments.
2. **At your discretion** — Authorize next reconciliation wave from the
   17 quarantined slices. Triage doc has the rubric.
3. **Before re-enabling watcher auto-dispatch** — Verify the new feature
   branches in `incident/reconcile-2026-05-03` history, or strategy-decide
   on the per-slice cherry-pick question.

## References

- `C:\Users\jgrif\.workshop\scripts\launch-agent-from-handoff.ps1` (D2 patch)
- `C:\Users\jgrif\.workshop\scripts\queue-watcher.ps1` (D3 patch)
- `C:\Users\jgrif\.workshop\queue\awaiting-jacob-pie\README.md`
- `C:\Users\jgrif\.workshop\queue\quarantine\2026-05-03-superseded-myikawave\README.md`
- `docs/adr/0005-queue-discipline-enforcement.md`
- `docs/runs/2026-05-03-watcher-incident.md`
- `docs/runs/2026-05-03-reconciliation-triage.md`
