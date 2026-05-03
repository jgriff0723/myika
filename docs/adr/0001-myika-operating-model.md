# ADR-0001: Myika operating model (G1–G5 lock)

**Status:** Accepted
**Date:** 2026-05-03
**Deciders:** Jacob Griffith (human), Claude (Myika)
**Source:** `/grill-me` session 2026-05-03 → `../../myika_plugin/GRILL_SUMMARY.md` (research sibling) and `../../ROADMAP.md`

## Context

The original `MyikaAI/myika-unreal` project (Tauri+React desktop chat + UE plugin bridge) was abandoned after over-engineering. We restarted in this repo (`myikai_plugin`) with a different shape: a single UE5.7 plugin that bundles open-world systems (water, sky, weather, landscape, PCG, foliage, footsteps, interaction, inventory, swimming, NPC AI), built natively from UE5.7 built-ins + free assets, with no marketplace dependencies.

The repo already had Codex-driven scaffolding (Combat/Platforming/SideScrolling Epic template variants under `Source/myikai_plugin/`) but no commitment to deliverable shape, multi-agent dispatch model, or scope ambition. A grill on 2026-05-03 forced those decisions and locked them.

## Decision

### G1 — Module shape: **plugin**

Plugin code and content live at:

- `Plugins/Myika/Source/{MyikaCore, MyikaSky, MyikaWater, MyikaLandscape, MyikaPCG, MyikaWeather, MyikaInteraction, MyikaSwap, MyikaItems, MyikaSwim, MyikaNPC, ...}/`
- `Plugins/Myika/Content/`

The `myikai_plugin` UE project becomes the **test harness**. Combat/Platforming/SideScrolling variants stay in the project as Epic-template fixtures, NOT in the plugin. This makes `Plugins/Myika/` portable to other UE5.7 projects.

### G2 — Hands-off: **end-of-vertical review only**

Agents finish a vertical (V1 Sky, V2 Water, …) → PIE proof → screen recording → Linear issue closure. Jacob reviews at vertical-complete checkpoints, not at every commit. Failures escalate immediately; successes accumulate silently.

### G3 — Parallelism: **rolling 2–3 verticals in flight**

After Phase 1 (shared core) lands, Phase 2 verticals run in parallel git worktrees with concurrency capped at 2–3. Manageable cognitive load if anything escalates; merges to `main` stay smooth. Within a vertical, sub-slices fan out the same way (e.g. V1-S3/S4/S5a/S6 are parallel-safe after V1-S2).

### G4 — Escalation budgets

- Build failure: escalate after **3 attempts**.
- PIE proof failure: escalate after **5 attempts**.
- Aesthetic judgment: **always** escalates (only humans judge looks).
- Wall-clock cap: **2 hours** per slice → escalate.

### G5 — Task source-of-truth: **Linear strategic + workshop queue tactical**

- **Linear `Unreal_myika`** project: one issue per vertical (V1, V2, …) and one issue per slice (V1-S1..S9). Reviewable, closeable, attachable PIE recordings.
- **Workshop queue** (`C:\Users\jgrif\.workshop\queue\`): per-step execution locks during build. Prevents two agents touching shared assets (e.g. `MPC_Myika`, master materials) simultaneously.

Two layers, different jobs. Queue task frontmatter mirrors the linked Linear issue via `linear_issue` / `linear_url` / `linear_status`.

## Foundational decisions inherited from the grill

These are not in scope to revisit casually. Update via a successor ADR.

| # | Locked answer |
|---|---------------|
| Q1 product type | AI-powered UE5.7 plugin product |
| Q2 marketplace relationship | AI **replaces** marketplace plugins; uses UE built-ins only |
| Q3 differentiator | Open-world generation depth |
| Q4 scope | Full game eventually (env + gameplay + UI + sequencer + cinematics) but ship environment-first via V1-V5 |
| Q5 interaction model | Scaffold-then-iterate |
| Q6 agent shape | Multi-agent specialists *(future direction; v1 is single-Claude-per-vertical)* |
| Q7 run context | Edit-time only |
| Q8 model strategy | Multi-provider; Claude family default tiered (Opus/Sonnet/Haiku); Codex/GPT swappable per agent |
| Q9 team | Solo Jacob + workshop agents in parallel worktrees |
| Q10 timeline | AI-accelerated; non-constrained |
| Q11 license | OSS (MIT recommended) |
| D1 plugin language | Hybrid C++ + Python *(MyikaCore in C++; Python comes when AI-tooling layer lands)* |

## Consequences

**Positive:**
- Clear deliverable shape gates every implementation choice (plugin module structure, content location, asset naming conventions).
- Auto-dispatch can claim queue tasks without human gating most of the time.
- Each vertical is independently shippable + reviewable; failure of one doesn't block others.
- Open-source posture means contributor friction is minimized (clean module boundaries, no marketplace dep entanglements).

**Negative:**
- 11 verticals × ~9 slices each = ~100 queue tasks to manage. Requires healthy queue daemon hygiene.
- Aesthetic-always-escalates rule means Jacob is the bottleneck for every visual pass. Acceptable for now; may need an aesthetic-judge agent later.
- Multi-agent specialist orchestration is deferred — single-Claude per worktree is fine for v1 but won't scale to all 11 verticals running concurrently. Re-evaluate after V1+V2 ship.
- No git remote means there's no off-machine backup yet. Add GitHub remote before V1 ships.

**Neutral:**
- Codex's prior Linear-based backlog conventions are preserved — no thrash on `docs/agents/*`.
- Existing Combat/Platforming/SideScrolling Epic variants stay in the project as test fixtures; they don't become part of the plugin deliverable.

## Follow-ups

- ADR-0002: pick license officially (recommend MIT) and add `LICENSE` file at repo root.
- ADR-0003: pick git remote host (GitHub Myikaai org? new myika org?) and push initial commit.
- Future ADR: when single-Claude per worktree hits a ceiling, decide on multi-agent specialist routing.
- Future ADR: AI-tooling layer (when Phase 3 skill wrappers start) — Python-in-UE vs external orchestrator pattern.

## References

- `../../ROADMAP.md` — Phase 0/1/2/3 plan
- `../../myika_plugin/GRILL_SUMMARY.md` — full grill transcript and reconciliation against the legacy `MyikaAI/myika-unreal/` project
- `../../myika_plugin/_chunk_*.md` — competitive plugin research (22 chunks)
- `../agents/backlog.md` — Linear + workshop queue conventions (pre-dates this ADR; kept as-is)
