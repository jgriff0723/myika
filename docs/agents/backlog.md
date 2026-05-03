# Backlog: Linear Project + Workshop Queue

Issues, PRDs, and execution tasks for this repo live in Linear project `Unreal_myika` under the `Myika AI` team. The shared workshop queue remains the local execution record for locks, handoffs, and validation evidence.

## Conventions

- Linear team: `Myika AI` (`MYI`)
- Linear project: `Unreal_myika`
- Linear project URL: `https://linear.app/myika-ai/project/unreal-myika-ece1cc43b11a`
- Canonical execution queue: `C:\Users\jgrif\.workshop\queue\pending\`, `active\`, and `done\`
- Task template: `C:\Users\jgrif\.workshop\queue\task-template.md`
- Repo field for this project: `C:\Users\jgrif\Documents\myikaai_plugin\myikai_plugin`
- Queue tasks must carry `linear_issue`, `linear_url`, and `linear_status` once the work is represented in Linear
- Myika / Claude owns planning, PRDs, issue shaping, and review-oriented backlog work
- Neco / Codex owns implementation, validation, and repo mechanics
- Teri / local Llama owns lightweight status checks, queue/board sync, and routing
- Triage state lives in Linear issue labels and may be mirrored in queue task notes for local routing
- `.scratch/` is not the authoritative backlog for this repo
- GitHub Issues are not the authoritative backlog for this repo unless the project is later connected and this file is updated

## When a skill says "publish to the backlog"

Create or update a Linear issue in project `Unreal_myika` under team `Myika AI`. For execution-ready work, also create or update a matching queue task under `C:\Users\jgrif\.workshop\queue\` and record the Linear link in frontmatter.

Include:

- a clear title
- the repo path
- target behavior
- constraints
- acceptance criteria
- validation notes
- linked Linear metadata when relevant
- routing labels such as `myika` for planning/review and `Neco` for implementation

## When a skill says "fetch the relevant ticket"

Read the referenced Linear issue first. If a local queue task exists for active execution, read that task next for local lock state, handoff details, and validation evidence.

The user may reference work by Linear issue key such as `MYI-123`, by Linear URL, or by queue task path.

## Delegation

- Myika / Claude: planning, PRDs, grilling, issue shaping, architecture decisions, reviews, acceptance criteria.
- Neco / Codex: implementation, refactors, build/test evidence, validation comments, status movement after proof.
- Teri / local Llama: status lookup, command routing, queue/Linear sync checks, lightweight board summaries.
- Cody and Wodi: brokered specialist labels only; do not invent fake Linear assignees for them.

## Notes

- Linear is the project control plane for this repo.
- The workshop queue is still the execution queue and local validation ledger.
- If a skill assumes GitHub issue commands or repo-local `.scratch/` notes, translate that intent into a Linear issue plus a linked queue task when execution starts.

## Roadmap reference

The current build plan lives in [`../../ROADMAP.md`](../../ROADMAP.md). It defines:

- The 4 phases (0 Foundation → 1 Shared core → 2 Verticals V1-V11 → 3 AI skill wrappers)
- The locked operating model (G1-G5, see [`../adr/0001-myika-operating-model.md`](../adr/0001-myika-operating-model.md))
- Per-vertical deliverable lists and `Haven.umap` integration plan

Queue task naming convention for vertical work: `YYYY-MM-DD-NNN-myika-v{N}-s{n}-{slug}.md` — example: `2026-05-03-002-myika-v1-s1-myikasky-module-scaffold.md`. Phase prerequisites (e.g. `MPC_Myika` asset) follow `YYYY-MM-DD-NNN-myika-phase{N.M}-{slug}.md`.
