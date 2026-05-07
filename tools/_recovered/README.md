# `tools/_recovered/` — Python scripts recovered 2026-05-05

These 17 scripts were authored on 2026-05-03 by parallel agents on the
`incident/reconcile-2026-05-03` branch. They were stashed (commits `21cc427`,
`182ccfe`) but never landed on `main` or `heal/sky-water-restore-2026-05-04`.
On 2026-05-05 (Claude, Myika lane) I restored them from the stash here so
Neco/Codex doesn't have to re-author from the queue specs.

## DO NOT BLINDLY RE-RUN

These scripts are the same ones that authored assets which then **crashed the
editor** in May-3 PIE attempts:

- `9cc3b31 fix(sky): quarantine new sky assets - editor hangs on L_SkyTest open`
  — sky materials, curves, cloud presets froze the editor on map open.
  Likely `M_MyikaStars` Noise expression or asset-registry churn.
- `d81ccbd fix(water): emergency rollback - quarantine M_MyikaWater rebuild`
  — UE process leaked memory 16 GB → 26 GB+ in 20 s with frozen log activity.
  Likely cross-plugin asset registry pathology
  (`/Myika MI` → `/Myika Material` → `/Myika texture`).

The root cause of both crashes is **not yet diagnosed**. Re-running these
scripts in their current form will probably reproduce both crashes. Don't
flip MYI-60 / MYI-67 status until either:

1. The crash is root-caused and a script fix is in place, OR
2. The asset is authored via a different path that demonstrably doesn't
   reproduce the editor freeze (e.g. moving `M_MyikaWater` into the engine
   Water plugin extension dir, or parenting it off
   `/Water/Materials/WaterSurface/Water_Material` instead of building from
   scratch — both options are listed in the `d81ccbd` commit message).

## What I patched

Only one script has been edited from its 2026-05-03 form:

### `Create-MyikaWaterAssets.py`

- `build_master_material()` — was logging a warning on
  `recompile_material → False`; now **raises `RuntimeError`**. This is the
  fix the MYI-67 bounce hint asked for. Without this, the script could write
  `{status: ok, recompile_ok: false}` to summary.json and the queue task
  would be marked complete despite a broken shader.
- `write_summary()` — `status` now reflects recompile outcome
  (`"fail"` when `recompile_ok=False`). Defense-in-depth in case a future
  refactor disables the raise.

All other recovered scripts are byte-for-byte from commit `182ccfe`.

## Inventory

```
Create-MyikaCausticAssets.py            # MYI-72 caustic decal (DBM_EMISSIVE fix per queue/2026-05-03-027 still pending in this copy)
Create-MyikaMpcAssets.py                # MYI-54 MPC asset (already Done; reference)
Create-MyikaWaterAssets.py              # MYI-67 water master + ocean/lake MIs (PATCHED — see above)
Validate-MyikaMpcAssets.py              # MPC sanity check used by other scripts
generate_myika_caustic_source.py        # PNG generator for caustic flipbook
generate_myika_water_textures.py        # PNG generator for water normal map
generate_myika_water_scaffold_assets.py # MYI-66 V2-S1 scaffold
inspect_myi70_assets.py                 # MYI-70 ripple solver inspector
validate_myika_water_materials.py       # post-author validation
validate_myika_water_scaffold.py        # MYI-66 PIE auto-validation (timed out; harness needs repair)
verify_myika_sky_assets.py              # post-author validation

python/author_celestial_materials.py    # M_MyikaStars + M_MyikaMoon (CRASH SUSPECT — Noise expression)
python/author_sky_curves.py             # the 6 empty curve assets from commit 2ec0650
python/author_water_materials.py        # earlier water authoring iteration
python/create_missing_assets.py         # the script that authored the 6 cloud presets + M_MyikaUnderwaterPP (CRASH SUSPECT for sky)
python/render_sky_time_sweep.py         # 24h time-of-day render harness
python/reparent_to_engine_water.py      # MI reparenting helper
```

## Suggested re-introduction order (after crash root-cause)

1. Run `Validate-MyikaMpcAssets.py` first — confirms the canonical MPC is
   what every other script assumes.
2. Try `Create-MyikaCausticAssets.py` on the dedicated `L_WaterTest` map
   only (smaller blast radius than `L_SkyTest`). If it compiles clean
   on SM6, MYI-72 becomes PIE-ready.
3. After diagnosing the SLW cross-plugin asset-registry leak, retry
   `Create-MyikaWaterAssets.py`. The patch above prevents silent failure.
4. After diagnosing the sky crash, retry `python/create_missing_assets.py`
   for cloud presets and `python/author_celestial_materials.py` for stars/moon.
5. `python/author_sky_curves.py` only creates empty curves anyway — keys
   must be authored by hand in editor (see commit `2ec0650`'s message).

## Why these were lost

The `incident/reconcile-2026-05-03` branch was the May-3 reconciliation work.
After the editor crashes, Jacob branched off to `heal/sky-water-restore-2026-05-04`
to recover. The heal branch reset `Plugins/Myika/Content/` to untracked and
never re-committed `tools/`. The compiled `.pyc` files survived in
`tools/__pycache__/` but the `.py` source did not.
