# Global Codex Bootstrap

## Identity (load FIRST every session)
You are part of a multi-agent workshop. Load these BEFORE any other workshop file:

1. Shared soul (values for all agents): `C:\Users\jgrif\.workshop\SOUL.md`
2. Your identity (who *you* specifically are): `C:\Users\jgrif\.workshop\identities\codex.md`

The soul is shared. Your identity file overrides voice but never values.
Speak as the persona in your identity file. Sign daily-log lines as `[CODEX]`.

## Workshop SOP
This agent is part of a two-agent system (plus local Llama). At the start of every session, after loading SOUL and identity:

1. Load and follow: `C:\Users\jgrif\.workshop\SOP.md`
2. Load routing rules: `C:\Users\jgrif\.workshop\ROUTING.md`
3. Load capabilities: `C:\Users\jgrif\.workshop\CAPABILITIES.md`
4. Load workflow: `C:\Users\jgrif\.workshop\WORKFLOW.md`
5. Load optimization rules: `C:\Users\jgrif\.workshop\OPTIMIZATION.md`
6. Load agent loadouts: `C:\Users\jgrif\.workshop\AGENT-LOADOUTS.md`

Task queue: `C:\Users\jgrif\.workshop\queue\`
Daily log: `C:\Users\jgrif\.workshop\logs\daily-[today].md`
Primer: `C:\Users\jgrif\.workshop\logs\primers\primer-[today].md`
Active handoff: `C:\Users\jgrif\.workshop\handoff\current.md`

When starting a session, read today's primer first if it exists. Then check the active handoff and pending queue before accepting new work.
Before `/clear` or `/compact`, run `/daily-memory` and write a compact session snapshot to `C:\Users\jgrif\.workshop\logs\daily-memory-[today].md`.

## Agent skills

### Backlog

This repo tracks project work in Linear project `Unreal_myika` under the `Myika AI` team, with the shared workshop queue used for local execution locks, handoffs, and validation records. See `docs/agents/backlog.md`.

### Triage labels

This repo uses the default Matt Pocock skill triage vocabulary: `needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, and `wontfix`. See `docs/agents/triage-labels.md`.

### Domain docs

This repo uses a single-context Unreal Engine project layout rooted at `myikai_plugin.uproject`, with optional `CONTEXT.md` and `docs/adr/` docs as they are introduced. See `docs/agents/domain.md`.

## Mandatory pre-implementation reads

Before generating any code or content for a Myika slice, an agent MUST read:

1. **`ROADMAP.md`** — the 4-phase plan and where this slice fits.
2. **`docs/adr/0001-myika-operating-model.md`** — G1-G5 lock (module shape, hands-off, parallelism, escalation, task layers).
3. **`docs/adr/0004-bp-slider-discipline.md`** — **REQUIRED** style for every BP/component/UPROPERTY. Code reviewer rejects PRs that miss it.
4. **`Plugins/Myika/Content/ASSET-ONBOARDING.md`** — naming conventions + license whitelist for any new asset added to the plugin.
5. The slice's **referenced chunk file(s)** under `../myika_plugin/_chunk_NN_*.md` — the recipe being implemented.
6. **`docs/adr/0003-git-remote-and-initial-commit.md`** — third-party content quarantine rules.

## Hard constraints (cross-vertical)

- **No marketplace plugin assets.** Build natively from UE5.7 built-ins. Reference chunks are informational only.
- **No quarantined content paths in commits.** `Content/Materials/`, `Content/Characters/`, `Content/Landscape/`, `Content/Water/`, `Content/Terrain/`, `Plugins/GaeaUnrealTools/` are git-ignored. Don't un-ignore without provenance audit.
- **License whitelist** (per ASSET-ONBOARDING.md): CC0, CC-BY, MIT, Apache, public-domain, Jacob's own work, UE Engine content. Megascans = local only, never commit.
- **Escalation budgets:** 3 build attempts / 5 PIE attempts / aesthetic always escalates / 2hr wall-clock. Hit the cap → drop a comment on the Linear issue and exit cleanly.

## Mandatory in-task discipline (per ADR-0005)

These are NOT optional. Agents that skip them violate the operating model and damage Linear/queue trust:

### Linear sync (D4)

- **Within 30 sec of claim:** transition the task's `linear_issue:` from Backlog → In Progress.
- **On every escalation:** transition to In Review and post a comment with the failure mode + escalation budget consumed.
- **On completion:** transition to In Review with the PIE proof link as a comment / attachment.
- **NEVER** transition Linear to "Done" — only Jacob does that after aesthetic sign-off.

Until ADR-0005 D9 (watcher-side auto-transition) lands, this is the agent's responsibility. Use the Linear MCP `save_issue` tool with `state` parameter.

### Per-slice commit discipline

- Work in the worktree the watcher created at `<repo>/../worktrees/<task-id>` on the `branch:` in your task frontmatter. **Never** check out a different branch in that worktree.
- Commit incrementally as you make progress. Don't accumulate >50 changed files in an uncommitted state.
- Final commit message format: `feat(<system>): MYI-NN <slice title>` — example: `feat(sky): MYI-55 V1-S1 MyikaSky module scaffold`.
- Do NOT use `--no-verify` unless explicitly authorized. Whitespace failures are fixable mechanically.

### Acceptance gate (D5)

Before marking your task done (`active/ → done/`):

1. **All `## Acceptance Criteria` checkboxes flipped to `- [x]`** — if you can't honestly check one, escalate, don't mark done.
2. **`pie_proof:` frontmatter populated** with a real path or URL — screen recording, screenshot, Linear attachment, run-log path.
3. **`## Out of Scope` section exists** in the task body confirming you stayed in your lane.
4. **Run log filed** at `docs/runs/YYYY-MM-DD-myi-NN-<short-name>.md` per template.
5. **Linear issue transitioned** to In Review with the proof attached.

If any of these is missing, the task stays in `active/` and you must address it before retry.

### Conflict awareness (D3)

- The `touches:` field in your task frontmatter declares the files/globs you're allowed to write to.
- If your work needs to write outside that list, escalate (don't quietly expand scope) — the watcher's conflict detection assumed you'd stay in the declared paths.
- The `serializes_with:` field signals shared-resource lockouts. If you see `serializes_with: [shared-core]` on your task, you can assume no concurrent task is modifying MyikaCore. Trust it; don't double-check by reading files outside your `touches:` set.

### Run-log discipline

Every coding session that edits files MUST produce one entry under `docs/runs/`. Template:

```markdown
# MYI-NN / <slice-id> <title>

Date: YYYY-MM-DD
Repo: <repo path>
Task: <queue task path>
Branch: <branch>
Worktree: <worktree path>

## Code Landed In This Run
- list of files added / modified

## Validation
- editor compile result
- game compile result
- PIE result + recording path

## Linear
- transitioned <state> -> <state> at <timestamp>
- proof attached: <link>

## Out of Scope
- explicit list of what this task did NOT touch

## Honest Acceptance Read
- per-criterion: met / not met / partial
```

This is mandatory per ADR-0005 — without a run log, the acceptance gate fails.

## Repo shape

- Project file: `myikai_plugin.uproject`
- C++ source and modules: `Source/`
- Project config: `Config/`
- Assets and maps: `Content/`
- Generated/runtime output: `.vs/`, `Binaries/`, `Intermediate/`, and `Saved/`

Treat generated/runtime output as non-authoritative unless the task explicitly requires touching it.

## Git safety

Do not run destructive or external git operations without explicit human approval. This includes `git push`, `git reset --hard`, `git clean -f`, `git clean -fd`, `git branch -D`, `git checkout .`, and `git restore .`.
