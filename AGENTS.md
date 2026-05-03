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

## Repo shape

- Project file: `myikai_plugin.uproject`
- C++ source and modules: `Source/`
- Project config: `Config/`
- Assets and maps: `Content/`
- Generated/runtime output: `.vs/`, `Binaries/`, `Intermediate/`, and `Saved/`

Treat generated/runtime output as non-authoritative unless the task explicitly requires touching it.

## Git safety

Do not run destructive or external git operations without explicit human approval. This includes `git push`, `git reset --hard`, `git clean -f`, `git clean -fd`, `git branch -D`, `git checkout .`, and `git restore .`.
