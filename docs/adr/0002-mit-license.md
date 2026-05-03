# ADR-0002: License — MIT

**Status:** Accepted
**Date:** 2026-05-03
**Deciders:** Jacob Griffith (human), Claude (Myika)
**Supersedes:** none
**Superseded by:** none

## Context

ADR-0001 locked myika as **free / OSS** (Q11 from the grill). License choice was deferred to a follow-up. The two reasonable options for an OSS UE plugin distributed via GitHub + free Fab:

- **MIT** — minimal, permissive, widely understood, allows commercial fork without copyleft entanglement.
- **Apache 2.0** — also permissive, adds explicit patent grant + contributor terms; stronger for corporate adoption but higher legal complexity.

Reference: the prior `MyikaAI/myika-unreal/` project also chose MIT (`README.md` line 92). Continuity argues for the same choice; nothing in this rebuild changes that calculus.

## Decision

**MIT License.** Copyright holder: **Jacob Griffith**. Year: **2026**.

`LICENSE` file added at the repo root with the standard SPDX MIT text.

## Rationale

- **Lowest friction** for adoption (forks, studio use, marketplace co-distribution).
- **Compatible with bundled UE Engine content** under Epic's EULA (UE assets we depend on — Niagara templates, Sky Atmosphere, etc. — are governed by the EULA, not by our license).
- **Compatible with free third-party assets** we'll bundle (Megascans is free for UE; CC0 textures from NASA / freesound; Apache/MIT/CC0 community packs).
- **Patent grant gap** is acceptable for a content/Blueprint-heavy plugin — none of the techniques here (PCG biome scattering, master shader macros, MPC sync) involve patentable algorithms.
- **Continuity** with the prior `myika-unreal` repo's MIT choice avoids cross-repo license confusion.

## Consequences

**Positive:**
- Anyone can fork, modify, sell, sublicense, and bundle. Maximizes reach.
- No copyleft contamination if a future contribution depends on a permissive third-party library.
- Standard SPDX identifier (`MIT`) makes Fab marketplace listing straightforward.

**Negative:**
- A bad actor could fork, rename, and sell as proprietary without contribution back. Acceptable trade for adoption velocity.
- No explicit patent grant. Acceptable per rationale above.

**Neutral:**
- All future contributors implicitly license their contributions under MIT by submitting PRs.

## Inbound contribution policy

Contributors agree that any code/content submitted is licensed under MIT. We do **not** require a CLA for v0.x; revisit if/when a CLA becomes necessary for studio adoption (likely never for a permissive license like MIT).

## Implementation

- [x] Write `LICENSE` at repo root with MIT text + Jacob Griffith / 2026.
- [ ] Once a `README.md` lands at repo root, add a "License" section pointing to `LICENSE`.
- [ ] When a remote is configured (ADR-0003), GitHub will auto-detect MIT from the `LICENSE` file and surface it on the repo overview.

## References

- ADR-0001 §Q11 (license decision parent)
- `LICENSE` (this repo's root)
- SPDX identifier: `MIT`
- Reference repo: `https://github.com/Myikaai/myika-unreal` (prior project, also MIT)
