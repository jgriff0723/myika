# ADR-0005: Queue dispatch discipline — enforcement, not aspiration

**Status:** Accepted
**Date:** 2026-05-03
**Deciders:** Jacob Griffith (human), Claude (Myika)
**Triggered by:** `docs/runs/2026-05-03-watcher-incident.md`

## Context

ADR-0001 G2-G5 defined an operating model: end-of-vertical hands-off review, rolling 2-3 verticals in flight, tiered escalation budgets, Linear-strategic + queue-tactical task tracking. None of this was machine-enforced. The first time the watcher ran (2026-05-03), it ignored every constraint:

- Claimed 20 tasks at once (cap was supposed to be 2-3)
- Spawned 17 codex agents in one shared working tree (no worktree isolation)
- Agents collided on shared files, broke the build
- No Linear issue was transitioned out of Backlog
- No task moved from `active/` to `done/`
- No PIE proofs captured
- No commits made until incident response

This ADR locks the **enforcement mechanisms**. ADR-0001 documented the *what*; this ADR locks the *how-to-not-violate-it*.

## Decisions

### D1 — Concurrency cap is enforced by the watcher

**Rule:** `queue-watcher.ps1` MUST NOT claim a new task if `count(queue/active/) >= MaxInFlight`.

`MaxInFlight` defaults to **2**. Configurable via watcher CLI param. Counts only entries with target_agent in `{nceo, myika}`; ignores `target_agent: any` (those don't auto-dispatch).

This implements ADR-0001 G3 ("rolling 2-3 verticals in flight").

### D2 — Per-task worktree isolation is enforced by the watcher

**Rule:** When the watcher claims a task with a `branch:` field, it MUST:
1. Create a git worktree at `<repo>/../worktrees/<task-id>` on that branch (creating the branch from `main` if it doesn't exist).
2. Pass the worktree path as the working directory to the spawned agent process.
3. After task completion (success OR escalation), schedule worktree teardown after Jacob's review (NOT immediately — leaves evidence available).

If a task has no `branch:` field, the watcher refuses to claim it (forcing all tasks to declare their branch).

This prevents the "all 17 agents writing to the same tree" failure mode.

### D3 — File-conflict detection blocks concurrent collisions

**Rule:** Each queue task MUST declare a `touches:` field (YAML list of paths/globs) and an optional `serializes_with:` field (list of task IDs or tag groups).

Watcher logic on poll:
1. For each candidate task in `pending/`:
   - Read its `touches:` and `serializes_with:`
   - Compare against the union of `touches:` of all tasks currently in `active/`
   - If ANY path overlap (substring or glob match) → defer this task
   - If `serializes_with:` matches any in-flight task's `id` or shared tag → defer
2. Only claim non-conflicting candidates, up to `MaxInFlight`

Required `touches:` patterns to declare:
- Specific files: `Plugins/Myika/Source/MyikaCore/Public/MyikaTypes.h`
- Module folders: `Plugins/Myika/Source/MyikaSky/**`
- Shared assets: `Plugins/Myika/Content/MyikaCore/MPC_Myika.uasset`
- Top-level configs: `Plugins/Myika/Myika.uplugin`

Common serialization tags:
- `serializes_with: [shared-core]` — anything touching MyikaCore module
- `serializes_with: [uplugin]` — anything modifying `Myika.uplugin`
- `serializes_with: [haven]` — anything touching `Content/Terrain/Haven.umap`

### D4 — Linear status sync is mandatory and verifiable

**Rule:** The agent MUST transition the Linear issue named in `linear_issue:`:
- **On claim** (within 30 sec of starting): Backlog → In Progress
- **On completion gate hit** (build/PIE/aesthetic): In Progress → In Review (with PIE proof attachment OR escalation comment)
- **Never** mark Linear "Done" — only the human (Jacob) does that after sign-off

Verification: when the watcher tries to move a task `active/ → done/`, it queries Linear for that issue's current status. If status is not "In Review" or "Done", the move is refused and the agent's run log is flagged.

### D5 — Acceptance criteria gate is machine-checked before `done/`

**Rule:** Before any task moves `active/ → done/`:
1. All `## Acceptance Criteria` checkboxes in the task body MUST be `- [x]` (not `- [ ]`).
2. The task body MUST contain at least one of:
   - `## Validation` section with a real evidence link (PIE recording path, screen recording URL, Linear attachment URL, run log path)
   - A `pie_proof:` frontmatter field with the recording path
3. Task body MUST contain a `## Out of Scope` section confirming the agent acknowledged the boundary.

Failure of any gate → task stays in `active/` and the agent must address before retry.

### D6 — Watcher runs all spawned agents under a heartbeat sidecar that is monitored

**Rule:** Every spawned agent process MUST have its `agent-heartbeat-sidecar.ps1` running for the duration. Watcher monitors heartbeat freshness (configurable, default 60 sec). If heartbeat goes stale:
1. Watcher logs `[STALE]` event to daily log + workshop dashboard
2. After grace period (default 2× heartbeat interval), watcher kills the agent process AND its sidecar
3. Task moves `active/ → escalated/` with a stale-heartbeat note appended

This catches "agent stuck/crashed but task never released" failure mode.

### D7 — Watcher emits a status surface

**Rule:** Watcher MUST write a JSON status snapshot every poll cycle to `~/.workshop/state/queue-watcher-status.json`:
```json
{
  "updated_at": "2026-05-03T18:55:00Z",
  "max_in_flight": 2,
  "in_flight": [
    {"task_id": "2026-05-03-002", "branch": "feature/myika-v1-s1-sky-scaffold",
     "linear": "MYI-55", "claimed_at": "...", "heartbeat_age_sec": 12,
     "worktree": "C:\\...\\worktrees\\2026-05-03-002"}
  ],
  "deferred": [
    {"task_id": "2026-05-03-009", "reason": "blocks_with: 2026-05-03-001 (in flight)"}
  ],
  "pending_count": 18,
  "quarantined_count": 0
}
```

The localhost workshop dashboard can render this. At minimum, Jacob can `cat` the file to see queue state at a glance.

### D8 — First-claim smoke test before full dispatch

**Rule:** Whenever the watcher is started after being stopped (incl. cold start), it runs in **single-task probe mode** for the first poll cycle: claim one task, wait for completion, verify Linear sync + acceptance gates, ONLY THEN switch to normal `MaxInFlight` mode.

Probe mode is gated by a flag file `~/.workshop/state/.queue-watcher-probe-clear`. After successful single-task validation the watcher writes this flag and switches to normal mode. To force probe again (e.g. after watcher patches), `rm` the flag.

### D9 — Tasks declare a Linear post-claim transition handler

**Rule:** Each task MUST include a `linear_on_claim_state: "In Progress"` and `linear_on_complete_state: "In Review"` field. Watcher uses these to call Linear's GraphQL API (via `mcp__6183b417-*__save_issue` equivalent) on claim and completion. Agent doesn't have to remember.

If the task lacks these fields, the watcher refuses to claim (forcing migration of all task templates).

### D10 — All ADR-0001/0004/0005 constraints are loaded into agent context

**Rule:** `launch-agent-from-handoff.ps1` MUST inject `--system-prompt` content (or equivalent for the target agent CLI) that includes:
- Pointer to AGENTS.md
- Pointer to all `docs/adr/000N-*.md` files
- Pointer to the specific queue task being claimed
- The hard escalation budgets (3 build / 5 PIE / aesthetic always / 2hr)

This prevents the "agent didn't read the ADRs" failure mode by making them part of the agent's bootstrap, not optional reading.

## Consequences

**Positive:**
- The 2026-05-03 incident pattern becomes structurally impossible
- Watcher state is observable (D7) — Jacob can see what's happening at a glance
- Linear stays in sync automatically (D9) — no human-discipline dependency
- Acceptance gates are machine-enforced (D5) — agents can't "ship" broken work
- File-conflict detection (D3) prevents the most common collision pattern
- Probe mode (D8) gives a sanity check before every full dispatch wave

**Negative:**
- Task template grows new required fields (`touches`, `serializes_with`, `linear_on_claim_state`, `linear_on_complete_state`)
- Existing 20 quarantined tasks need migration to the new template before re-dispatch
- Watcher patch is non-trivial (worktree management + Linear API calls + JSON status)
- More upfront friction per task (declaring `touches:` correctly)

**Neutral:**
- These rules don't change the architecture from ADR-0001; they enforce it

## Implementation order (this session)

1. ✅ Postmortem (`docs/runs/2026-05-03-watcher-incident.md`)
2. ✅ This ADR
3. ⏳ Patch `queue-watcher.ps1` with D1, D2 (cap + worktree). D3-D9 are larger and tracked as follow-ups.
4. ⏳ Patch `~/.workshop/queue/task-template.md` with new mandatory fields (D3 + D9)
5. ⏳ Patch repo `AGENTS.md` with the in-task agent obligations (Linear sync, acceptance gates, escalation budgets)
6. ⏳ Linear comments on MYI-53..74 explaining incident + linking postmortem

## Implementation follow-ups (next session(s))

- Full D3 conflict-detection logic (current quick patch is `MaxInFlight` only)
- D4 status verification before done-move
- D5 machine-check of acceptance criteria
- D6 heartbeat-stale auto-kill
- D7 JSON status emission + localhost dashboard widget
- D8 probe mode flag
- D9 watcher-side Linear transitions (currently agents are responsible)
- D10 agent-bootstrap injection

## References

- ADR-0001 (operating model — context for what we're enforcing)
- ADR-0004 (BP slider discipline)
- `docs/runs/2026-05-03-watcher-incident.md`
- `C:\Users\jgrif\.workshop\scripts\queue-watcher.ps1`
- `C:\Users\jgrif\.workshop\queue\task-template.md`
