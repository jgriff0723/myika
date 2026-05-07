# Batched PIE Session — 2026-05-05

**Goal:** verify in one ~30-minute editor session every In-Review item that survived static review, plus the survivors of `awaiting-jacob-pie`. Items bounced to Neco in the static review (`docs/reviews/2026-05-05-in-review-batch.md`) are NOT in this script and should not be PIE-tested today.

**Excluded (bounced to Neco):**
- MYI-60 (no preset uassets on disk)
- MYI-67 + queue task `2026-05-03-026` (water material not present, recompile_ok=false)

**Order:** dependency-respecting. Each step is gated — if it fails, **stop**, log the failure under "Failure cases" below, and skip the remaining dependent steps.

**Conventions:**
- "Open `<map>`" = File → Open Level → `<map>`
- "Apply Lighting Build" not needed for any of these — runtime state.
- Log every PIE result in `docs/runs/2026-05-05-batched-pie-session.md` (this file) under each step.

---

## 1. MPC chain sanity (V1 prerequisite)

**Issue:** MYI-54 is already Done; this is a sanity check, not a verification.

1. Open `Plugins/Myika/Content/MyikaCore/Debug/M_MPCDebug` (if it exists per `MYI-54-mpc/generation.json`). It should expose all 7 scalars + 3 vectors as visible swatches/ramps.
2. **Expected:** all parameter names match (`TimeOfDay`, `Wetness`, `SnowCoverage`, `Temperature`, `LightningFlash`, `StormIntensity`, `WaterLevel`, `SunDirection`, `MoonDirection`, `WindVector`).
3. **On fail:** stop the whole PIE batch — every downstream item depends on this MPC.

---

## 2. MYI-55 — V1-S1 sky scaffold

**Issue:** [MYI-55](https://linear.app/myika-ai/issue/MYI-55) — keep In Review until this passes; on pass, move to Done.

1. Open `Plugins/Myika/Content/MyikaSky/L_SkyTest`.
2. **Expected:** map opens, blue sky visible, no error spam in OutputLog. `BP_MyikaSky` (or `AMyikaSkyActor`) is in the world. `EUW_SkyDebug` debug widget can be opened.
3. PIE (Alt+P or Play in Selected Viewport).
4. **Expected:** PIE starts, character spawns, no crash, no compile error. SkyAtmosphere visible.
5. **On pass:** comment on MYI-55 with a screenshot of L_SkyTest in PIE; transition to Done.
6. **On fail:** capture OutputLog tail and file the error under "Failure cases" below; bounce MYI-55 back to In Progress with the log.

---

## 3. MYI-66 — V2-S1 water scaffold

**Issue:** [MYI-66](https://linear.app/myika-ai/issue/MYI-66) — keep In Review until this passes; on pass, move to Done.

1. Open `Plugins/Myika/Content/MyikaWater/L_WaterTest`.
2. **Expected:** `WaterController` actor present, `WaterZone` actor present, default ocean visible (even if material is wrong/white — that's MYI-67's problem, not MYI-66).
3. PIE.
4. **Expected:** PIE starts, no crash, no compile error from the MyikaWater module loading.
5. **On pass:** comment on MYI-66 with screenshot; transition to Done.
6. **On fail:** capture log; bounce.

---

## 4. (skipped — MYI-67 bounced to Neco)

The water master material does not exist on disk. Skip until Neco re-runs `awaiting-jacob-pie/2026-05-03-026` with the recompile_ok check enforced.

---

## 5. MYI-72 — V2-S7 caustic decal SM6 compile

**Issue:** [MYI-72](https://linear.app/myika-ai/issue/MYI-72) + queue task `awaiting-jacob-pie/2026-05-03-027`.

1. With `L_WaterTest` open from step 3, drag `Plugins/Myika/Content/MyikaWater/Decals/M_MyikaCaustic.uasset` into the level (or place an `AMyikaCausticDecalActor`).
2. Watch OutputLog for the message `Failed to compile Material for platform PCD3D_SM6, Default Material will be used`. **That message must be absent.**
3. **Expected:** the decal projects animated caustic patterns onto a surface below the water plane; emissive contribution visible.
4. **On pass:** comment on MYI-72 + move queue task `2026-05-03-027` from `awaiting-jacob-pie/` → `done/`. Transition MYI-72 to Done.
5. **On fail:** capture the SM6 error log, bounce MYI-72 back to In Progress, and route the queue task back to Neco.

---

## 6. MYI-59 — V1-S5a cloud master material

**Issue:** [MYI-59](https://linear.app/myika-ai/issue/MYI-59).

1. Reopen `L_SkyTest`.
2. Inspect `BP_MyikaSky` (or the `AMyikaSkyActor` in the level) → set `DefaultCloudMaterial` to `MI_MyikaCloud_Default` if not already.
3. Confirm the world's `VolumetricCloudComponent` has `M_MyikaCloud` (via the MID at runtime).
4. PIE.
5. **Expected:** clouds render with sane Coverage / Morphology defaults; no shader compile error in OutputLog.
6. **Aesthetic call:** are clouds plausible (not flat, not overdriven)? If yes, pass. If they look broken, capture screenshot + bounce.
7. **On pass:** screenshot + transition MYI-59 to Done.

---

## 7. MYI-60 — skipped (bounced to Neco; no preset uassets)

---

## 8. MYI-62 — V1-S8 MPC sync chain

**Issue:** [MYI-62](https://linear.app/myika-ai/issue/MYI-62) — load-bearing. Last on purpose: a fail here is critical for V1.

1. With `L_SkyTest` open in PIE, open `EUW_SkyDebug` widget.
2. Scrub the `Time of Day` slider from 0 → 24.
3. **Expected — MPC is being driven:**
   - The sun rotates as the slider moves (proves `SunDirection` MPC vector writes).
   - If a debug material that samples MPC scalars exists (`M_MPCDebug` or similar from MYI-54), drop it into the level — its color/value swatches should change as the slider moves (proves the scalar-write loop works).
4. **Expected — no log spam:** `LogMyikaMPCSyncComponent` warnings should appear AT MOST once at PIE start (if MPC asset can't load), not on every Tick.
5. **On pass:** screenshot the time-of-day sweep and the live MPC values; transition MYI-62 to Done. **This unblocks V1-S2/S3/S4/S5b/S7 to move to In Review when their respective sub-features get their visual sweep.**
6. **On fail:** this is the most important failure to log fully. Capture:
   - OutputLog tail with any `LogMyikaMPCSyncComponent` lines.
   - Whether `MPC_Myika.MPC_Myika` exists at the package path the component expects (`/Myika/MyikaCore/MPC_Myika`).
   - Whether `UMyikaSubsystem` is initialized (look for `LogSubsystemCollection` Init lines for `MyikaSubsystem`).
   - File a fresh comment on MYI-62 with the log; transition back to In Progress.

---

## Failure cases (fill in during the run)

> One subsection per failure. Include OutputLog tail, screenshot path, and the queue/issue you're bouncing.

(empty — populate during PIE)

## Pass cases (fill in during the run)

> One line per pass: `MYI-XX  ✅  <screenshot path>  <one-line note>`

(empty — populate during PIE)

---

## After the session

If the run goes mostly green:

- **MYI-55, MYI-66, MYI-72, MYI-59, MYI-62** — moved to Done. **5 V1/V2 issues closed in one PIE round.**
- Several In-Progress V1 sub-issues (MYI-56 / 57 / 58 / 60 / 61 / 63) become *trivial to move to In Review* afterward, because S1+S8 passing means their structural slots in `AMyikaSkyActor` are demonstrably wired. Jacob (or Neco) can do that as a separate session — not this one.
- File V1-S9 closure issue `MYI-64` planning here as a follow-up if 5/5 land.

If MYI-62 fails:

- Treat as a critical-path stall. V1 cannot land. Re-prioritize on the project comment.
