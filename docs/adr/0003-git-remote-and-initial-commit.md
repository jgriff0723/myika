# ADR-0003: Git remote + initial commit

**Status:** Proposed (awaiting Jacob's go on remote URL)
**Date:** 2026-05-03
**Deciders:** Jacob Griffith (human), Claude (Myika)

## Context

The repo is git-initialized (`main` branch exists) but has **no remote configured** and **no commits yet** — every file (including `myikai_plugin.uproject`, the Plugins/Myika scaffold, AGENTS/CONTEXT/ROADMAP, ADRs 0001-0002, all 11 queue task references) is currently untracked. Until both are fixed:

- Off-machine backup is zero. A drive failure loses the entire project including ADR-0001's grilled decisions.
- Worktrees per ADR-0001 G3 ("rolling 2-3 verticals in flight") need a real branch graph to fan out from. Untracked files can't branch.
- `superpowers:using-git-worktrees` and the workshop git-guardrails hooks require a populated repo to be useful.
- Linear ↔ commit linkage in queue tasks (planned for the V1 dispatch flow) presumes commit SHAs exist.

## Decision (proposed)

### Remote: GitHub `Myikaai/myika`

Recommended remote URL: `git@github.com:Myikaai/myika.git` (or HTTPS variant).

Rationale:
- The prior project lived at `github.com/Myikaai/myika-unreal`. Same org keeps the Myika namespace consolidated.
- Repo name **`myika`** (not `myika-unreal`) reflects the rebuild's fresh shape — a plugin product, not a desktop+plugin pair like the legacy.
- MIT license + OSS posture (ADR-0002) → public visibility from day one.

Alternatives considered:
- **GitHub `Myikaai/myikai_plugin`** — matches the local folder name. Rejected: folder name is a typo-aware artifact ("myikai" vs "myika"); product name should win.
- **A new GitHub org** (e.g. `myika-ai/`) — unnecessary org sprawl. Stick with `Myikaai/`.
- **GitLab / Codeberg / sourcehut** — Linear integration, GitHub Actions ecosystem, and Fab marketplace expectations all favor GitHub. No reason to deviate.
- **Local-only forever** — rejected: zero off-machine backup is unacceptable past the first day's work.

### Initial commit

Single root commit titled `chore: initial myika plugin scaffold` containing:
- `LICENSE` (MIT, ADR-0002)
- `AGENTS.md`, `CONTEXT.md`, `ROADMAP.md`
- `myikai_plugin.uproject`, `Source/`, `Config/`, `Content/` (Epic template baseline)
- `Plugins/Myika/` (the plugin scaffold from G1)
- `docs/adr/0001-0003-*.md`
- `docs/agents/{backlog,domain,triage-labels}.md`
- `Plugins/GaeaUnrealTools/` (third-party heightmap tool)
- `.gitignore` (UE5.7 standard ignores; **must be authored before commit**)
- `.gitattributes` (LFS for binary UE assets; UE recommends LFS for `.uasset` / `.umap` / `.fbx` / `.png` / `.wav`)

Sign-off: standard `Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>` per workshop convention.

### Branch protection (deferred)

Branch protection on `main` (require PR, require status checks) is **deferred** until V1 ships. Solo dev right now; protection adds friction without payoff.

## Consequences

**Positive:**
- Off-machine backup the moment the first push lands.
- Worktrees + parallel agents become viable per ADR-0001 G3.
- Public OSS visibility supports the Q3 differentiator narrative ("AI-built UE plugin").
- Standard GitHub features (Issues, PRs, Actions, Releases) become available — though Linear remains canonical per ADR-0001 G5.

**Negative:**
- LFS quota costs at scale (UE projects get heavy fast). Free tier is 1GB storage + 1GB/month bandwidth; will hit limits within the first few verticals. Plan: GitHub Pro ($4/mo) bumps LFS to 50GB/50GB which covers v1+v2 comfortably.
- Public source from day one means competitors (Rekall, AgenticLink, Unreal MCP Server) can see the plan. Acceptable per ADR-0001 (OSS is the chosen posture).

## Implementation (NOT executed yet — awaiting Jacob's go)

Per CLAUDE.md safety rules, Claude does not push to remotes or commit without explicit instruction. Steps that require Jacob:

1. **Create the GitHub repo** (`Myikaai/myika`, public, MIT auto-detect from `LICENSE`, no template).
2. **Confirm the remote URL** to use (SSH or HTTPS). SSH recommended.

Once Jacob confirms, Claude can:

3. Author `.gitignore` and `.gitattributes` (LFS config) — these need to land BEFORE the first commit.
4. `git lfs install && git lfs track "*.uasset" "*.umap" ...`
5. `git remote add origin <URL>`
6. `git add -A` (with the LFS-aware ignores in place)
7. `git commit -m "chore: initial myika plugin scaffold" ...`
8. `git push -u origin main`

## Follow-ups

- ADR-0004: GitHub branch protection policy (defer until first external contributor or after V1 ship).
- ADR-0005: GitHub Actions CI (UE compile + automation tests) — defer until V1 ship.
- ADR-0006: Release process (tags, changelog, Fab publishing) — defer until V1 ship.

## References

- ADR-0001 (operating model — context for why this matters)
- ADR-0002 (MIT license — what gets pushed)
- `LICENSE`
