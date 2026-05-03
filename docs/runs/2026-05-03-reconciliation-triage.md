# Reconciliation Triage — V1 + V2 slices on `incident/reconcile-2026-05-03`

**Date:** 2026-05-03 (Phase B reconciliation kickoff)
**Author:** Claude (Myika)
**Branch:** `incident/reconcile-2026-05-03`
**Build status:** ✅ Clean. Game + Editor targets both compile + link successfully (5 plugin modules).

## Premise

The 2026-05-03 watcher flood produced ~80 files of additive code across 5 plugin modules. All of it compiles. None of it has been:
- Validated in the editor (PIE proof)
- Verified against its slice's acceptance criteria
- Linked back to its Linear issue
- Moved to `queue/done/`

This doc tracks per-slice triage so the next session knows exactly what's salvageable, what needs PIE, and what needs re-do.

## Triage rubric per slice

For each of the 21 quarantined tasks (Phase 1.1 + V1-S1..S9 + V2-S1..S9):

| Status | Meaning | Next action |
|--------|---------|-------------|
| ✅ **Salvage-clean** | Code complete, structure matches spec, just needs PIE + Linear update | Manually open editor, verify acceptance criteria, capture PIE recording, transition Linear, move queue task to `done/` |
| ⚠️ **Salvage-partial** | Code is real but missing pieces (e.g. assets not authored, defaults not tuned) | Identify gap, file a follow-up sub-task on the same Linear issue, do remaining work, then PIE+Linear+done |
| 🔁 **Re-do** | Code is wrong/dead-end | Discard, queue task back to `pending/` after watcher patch |
| 🔒 **Blocked** | Depends on something not yet built | Mark blocker explicitly, leave in quarantine until unblocked |

## Per-slice triage (initial pass — verified by file existence on disk + run-log evidence; PIE not yet attempted)

### Phase 1.1 — `MPC_Myika.uasset` (queue 001 / MYI-54)

- **Status:** ✅ Salvage-clean
- **Evidence:** `Plugins/Myika/Content/MyikaCore/MPC_Myika.uasset` exists. Helper Python `tools/Create-MyikaMpcAssets.py` + `Validate-MyikaMpcAssets.py` exist.
- **Next:** Open editor → verify all 7 scalars + 3 vectors present + defaults match ADR-0005 spec → screenshot Details panel → attach to MYI-54 → Linear In Review → Jacob sign-off → close.
- **Risk:** Low. Asset is binary, can't tell quality from file system alone, but tools/ scripts exist for validation.

### V1-S1 — MyikaSky module scaffold (queue 002 / MYI-55)

- **Status:** ✅ Salvage-clean
- **Evidence:** Module compiles. `BP_MyikaSky.uasset` exists. `L_SkyTest.umap` exists. Place Actors registration likely in MyikaSkyEditor.
- **Next:** Open `L_SkyTest` → confirm `AMyikaSkyActor` placeable + drops with default sky → screen recording → Linear → done.
- **Risk:** Low.

### V1-S2 — Atmosphere stack + linear time→sun (queue 003 / MYI-56)

- **Status:** ✅ Salvage-clean (probable)
- **Evidence:** `MyikaSkyActor.cpp` shows full atmosphere stack (DirectionalLight + SkyAtmosphere + SkyLight + ExpHeightFog + VolumetricCloud + PostProcess) + Tick logic with Lightning + Cloud preset support. Subsystem reads + writes work.
- **Next:** Open editor → open `EUW_SkyDebug` (built by V1-S1?) or use Details panel → scrub TimeOfDay → confirm sun rotates → 15s recording.
- **Risk:** Low-medium. Need to verify `EUW_SkyDebug` actually exists and works.

### V1-S3 — Sun Position Calculator (queue 004 / MYI-57)

- **Status:** ⚠️ Salvage-partial (probable)
- **Evidence:** `SunPosition` plugin enabled in `Myika.uplugin`. Need to inspect `MyikaSkyActor.cpp` for `bAccurateMode` toggle + `USunPositionFunctionLibrary` calls.
- **Next:** Grep for `SunPosition` in MyikaSkyActor; if missing, file follow-up sub-task. If present, PIE-test 3 lat/long cases.
- **Risk:** Medium. Possibly skipped during the flood.

### V1-S4 — Curve atlas tuning (queue 005 / MYI-58)

- **Status:** 🔁 Re-do (probable)
- **Evidence:** No `Plugins/Myika/Content/MyikaSky/Curves/` folder visible in initial audit. Curve assets are `.uasset`s that need to be authored in the editor.
- **Next:** Re-do — author `CurveAtlas_MyikaSky` + 3 float curves manually OR via Editor Utility. **Aesthetic gate per ADR-0004 — Jacob sign-off required.**
- **Risk:** High. Asset-authoring + aesthetic tuning likely incomplete.

### V1-S5a — Cloud master material (queue 006 / MYI-59)

- **Status:** ⚠️ Salvage-partial
- **Evidence:** `MyikaCloudPreset.h` exists. No `M_MyikaCloud.uasset` visible in initial audit (Materials/ folder may not exist yet).
- **Next:** Inspect editor → if M_MyikaCloud exists, validate parameters; if not, author from scratch.
- **Risk:** Medium-high.

### V1-S5b — Cloud presets + LerpToPreset (queue 007 / MYI-60)

- **Status:** ⚠️ Salvage-partial
- **Evidence:** `UMyikaCloudPreset` UCLASS exists in header. Per `MyikaSkyActor.cpp` line ~84 there's preset-blending state. 6 PCP_*.uasset preset assets need to exist in `Plugins/Myika/Content/MyikaSky/Presets/Clouds/`.
- **Next:** Inspect content folder, author preset DataAssets if missing, PIE-test 6 transitions.
- **Risk:** Medium.

### V1-S6 — Stars + Moon (queue 008 / MYI-61)

- **Status:** ✅ Salvage-clean (probable)
- **Evidence:** `MyikaCelestialActor.h/.cpp` exist. Subsystem already had Moon setters. Need to verify M_MyikaStars + M_MyikaMoon material + textures.
- **Next:** Inspect content + PIE at midnight.
- **Risk:** Medium.

### V1-S7 — Lightning timeline (queue 010 / MYI-63)

- **Status:** ✅ Salvage-clean
- **Evidence:** `MyikaLightningComponent.h/.cpp` exist. `MyikaSkyActor::TriggerLightningNow()` + `GetLightningCooldownRemaining()` exist. Subsystem `SetLightningFlash()` + `SetStormIntensity()` setters added.
- **Next:** Verify thunder SFX + bolt mesh assets exist (likely missing — need to source CC0). PIE with StormIntensity=1.
- **Risk:** Medium. Audio + mesh assets likely missing per ASSET-ONBOARDING workflow.

### V1-S8 — MPC sync component (queue 009 / MYI-62)

- **Status:** ✅ Salvage-clean
- **Evidence:** `MyikaMPCSyncComponent.h/.cpp` exist in MyikaCore. Compiles. Subsystem broadcast already covers all fields including LightningFlash/StormIntensity/WaterLevel.
- **Next:** Verify MPC writes happen at runtime via debug material. PIE recording.
- **Risk:** Low.

### V1-S9 — Haven integration smoke (queue 011 / MYI-64)

- **Status:** 🔒 Blocked (depends on V1-S2..S8 complete)
- **Next:** After V1-S2..S8 are PIE-validated and closed, integrate `BP_MyikaSky` into `Content/Terrain/Haven.umap`, capture 90s recording, etc.
- **Risk:** Medium. Haven currently in quarantine (`Content/Terrain/` is gitignored); needs to be unwound for V1-S9.

### V2-S1 — MyikaWater module scaffold (queue 012 / MYI-66)

- **Status:** ✅ Salvage-clean
- **Evidence:** Module compiles. `MyikaWaterController.h/.cpp` + `MyikaWaterModule.h/.cpp` + `MyikaWaterEditor` stub all exist. `Plugins/Myika/Content/MyikaWater/ASSETS.md` filed.
- **Next:** PIE-load any test map → confirm AMyikaWaterController placeable. Then file follow-up: create `L_WaterTest.umap` (it's listed in queue task but not visible in content folder yet).
- **Risk:** Low.

### V2-S2 — BP_MyikaOcean + Single Layer Water (queue 013 / MYI-67)

- **Status:** ⚠️ Salvage-partial
- **Evidence:** `MyikaOceanActor.h/.cpp` exist. Material `M_MyikaWater` + `MI_MyikaWater_Ocean` + `MI_MyikaWater_Lake` likely missing (Materials/ folder not in initial audit).
- **Next:** Inspect editor; author missing materials; PIE-test ocean rendering with Lumen + Single Layer Water.
- **Risk:** High. Custom Single Layer Water shader is the slice's core deliverable; if missing, this is mostly re-do.

### V2-S3 — BP_MyikaRiver (queue 014 / MYI-68)

- **Status:** ⚠️ Salvage-partial
- **Evidence:** `MyikaRiverActor.h/.cpp` exist. `MI_MyikaWater_River` + foam decal + obstacle detection logic need verification.
- **Next:** Inspect editor; PIE-test river spline + obstacle foam.
- **Risk:** Medium-high.

### V2-S4 — BP_MyikaLake (queue 015 / MYI-69)

- **Status:** ✅ Salvage-clean
- **Evidence:** `MyikaLakeActor.h/.cpp` exist + run log explicitly confirms "implemented in code, not editor-verified" + lists what blocks the verification (missing M_MyikaWater + L_WaterTest).
- **Next:** Once V2-S2 materials land, drop lake spline in test map, PIE.
- **Risk:** Low (code-side); blocked on V2-S2 content.

### V2-S5 — Niagara shoreline ripple (queue 016 / MYI-70)

- **Status:** ⚠️ Salvage-partial
- **Evidence:** `MyikaShorelineRippleComponent.h/.cpp` exist. Per MYI-70 run log, the missing pieces are: NS_MyikaShorelineRipple, RT_MyikaShorelineRipple, M_MyikaWater ripple-sampling extension, L_WaterTest, BP_TestSwimmer wiring. **Helper script `tools/generate_myika_water_scaffold_assets.py` exists for asset generation.**
- **Next:** Run the helper script (requires editor boot) → author Niagara System → wire into M_MyikaWater → PIE.
- **Risk:** Medium.

### V2-S6 — Underwater PP + waterline (queue 017 / MYI-71)

- **Status:** ⚠️ Salvage-partial
- **Evidence:** `MyikaUnderwaterPostProcessComponent.h/.cpp` exist. `M_MyikaUnderwaterPP` material likely missing.
- **Next:** Author PP material; PIE-test dive.
- **Risk:** Medium.

### V2-S7 — Caustic decal flipbook (queue 018 / MYI-72)

- **Status:** ⚠️ Salvage-partial
- **Evidence:** `MyikaCausticDecalActor.h/.cpp` exist. `tools/Create-MyikaCausticAssets.py` + `tools/generate_myika_caustic_source.py` + `tools/Generated/MyikaWater/T_MyikaCaustic_Flipbook.png` exist. Material `M_MyikaCaustic` + UE asset import likely incomplete.
- **Next:** Run generator → import flipbook → author M_MyikaCaustic material → PIE underwater.
- **Risk:** Medium.

### V2-S8 — BP_MyikaBuoyancy + boat (queue 019 / MYI-73)

- **Status:** ⚠️ Salvage-partial
- **Evidence:** `MyikaBuoyancyComponent.h/.cpp` + `MyikaBoatPawn.h/.cpp` exist. Boat hull mesh likely missing (need to source CC0 — Kenney.nl boat pack noted in queue task).
- **Next:** Source CC0 boat mesh, import, attach to BP_MyikaBoat, PIE-test floating + driving.
- **Risk:** Medium. Asset-onboarding pipeline test.

### V2-S9 — Haven integration smoke (queue 020 / MYI-74)

- **Status:** 🔒 Blocked (depends on V2-S2..S8 complete)
- **Next:** After all V2 slices PIE-validated, integrate ocean+river+lake+boat into Haven, 90s recording.

## Summary table

| Status | Count | Slices |
|--------|-------|--------|
| ✅ Salvage-clean | 6 | Phase 1.1, V1-S1, V1-S2, V1-S7, V1-S8, V2-S1, V2-S4 |
| ⚠️ Salvage-partial | 9 | V1-S3, V1-S5a, V1-S5b, V1-S6, V2-S2, V2-S3, V2-S5, V2-S6, V2-S7, V2-S8 |
| 🔁 Re-do | 1 | V1-S4 (curves likely never authored) |
| 🔒 Blocked | 2 | V1-S9, V2-S9 (Haven integration) |

**Verdict:** Way better than expected. ~6 slices are essentially done modulo PIE+Linear formality. ~9 need an asset-authoring pass to complete. Only 1 needs a real re-do.

## Recommended next-session order

**Wave 1 — clean closures (quick wins, ~2 hr total)**
1. Phase 1.1 / MYI-54 — open editor, verify MPC, attach screenshot, close
2. V1-S1 / MYI-55 — open L_SkyTest, verify scaffold, recording, close
3. V2-S1 / MYI-66 — verify water module loads, recording, close

**Wave 2 — asset authoring + closures (~4-6 hr)**
4. V1-S2 / MYI-56 — verify atmosphere works in PIE
5. V1-S8 / MYI-62 — MPC sync + debug material proof
6. V2-S4 / MYI-69 — lake (depends on V2-S2 materials first)
7. V2-S2 / MYI-67 — author M_MyikaWater materials (aesthetic gate, slow)
8. V1-S5a + V1-S5b / MYI-59 + MYI-60 — author cloud materials + presets (aesthetic gate)
9. V1-S6 / MYI-61 — verify stars + moon assets
10. V1-S7 / MYI-63 — source CC0 thunder + bolt mesh

**Wave 3 — remaining V2 (~4-6 hr)**
11. V2-S3 / MYI-68 — river verification + foam
12. V2-S5 / MYI-70 — Niagara shoreline ripple authoring
13. V2-S6 / MYI-71 — underwater PP material
14. V2-S7 / MYI-72 — caustic flipbook (use existing generator)
15. V2-S8 / MYI-73 — source CC0 boat mesh

**Wave 4 — re-do (~2-4 hr)**
16. V1-S4 / MYI-58 — author curve atlas + 3 float curves with aesthetic sign-off

**Wave 5 — closures (~3-4 hr)**
17. V1-S9 / MYI-64 — Haven integration + 90s recording
18. V2-S9 / MYI-74 — Haven integration + 90s recording

**Estimated total reconciliation effort:** 15-22 hours of focused work, mostly editor-side (asset authoring + PIE), spread across 3-5 sessions.

## Process discipline going forward

Each per-slice closure must follow the post-incident playbook:
1. Linear: Backlog → In Progress on start
2. Validate against the queue task's `## Acceptance Criteria` checkboxes
3. Capture PIE recording (or screenshot for static deliverables)
4. Linear: In Progress → In Review with proof attached
5. Jacob aesthetic sign-off (for any aesthetic-gated slice — V1-S4/S5a/S5b/S6/S7/V2-S2/S3/S5/S6/S7)
6. Move queue task: `quarantine/2026-05-03-watcher-incident/<file>.md` → `queue/done/<file>.md`
7. Linear: In Review → Done (Jacob only)
8. Run log entry at `docs/runs/YYYY-MM-DD-myi-NN-<slice>.md`

## Open follow-ups for next session

1. Decide: keep working on `incident/reconcile-2026-05-03` until all 21 close, OR cherry-pick clean slices to per-slice branches as we go.
2. Decide: when (if ever) to merge `incident/reconcile-2026-05-03` to `main`. Likely after all 21 close.
3. Triage stale May 1 tasks in `~/.workshop/queue/active/` (5 left from earlier sessions, unrelated to V1/V2).
4. Land ADR-0005 D2 (worktree isolation in `launch-agent-from-handoff.ps1`) BEFORE re-enabling watcher auto-dispatch.
5. Land ADR-0005 D9 (watcher-side Linear auto-transition) so agents stop being responsible for it.
6. Run a single-task probe smoke test through the patched watcher before reopening to MaxInFlight=2.

## References

- `docs/runs/2026-05-03-watcher-incident.md` — postmortem
- `docs/adr/0005-queue-discipline-enforcement.md` — enforcement contracts
- Commits: `c1241a1` (initial) → `21cc427` (WIP preserve) → `cee5397` (incident-fix)
- Branch: `incident/reconcile-2026-05-03`
- GitHub: https://github.com/jgriff0723/myika
- Linear epics: MYI-53 (V1) + MYI-65 (V2)
