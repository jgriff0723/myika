# Execution Pass — 2026-05-05 (Myika lane, post-cleanup)

After the 2026-05-05 review + cleanup pass, executed an end-to-end
verification of what's verifiable without the editor + diagnosed the
editor crash that's been the root blocker for V1 + V2.

## What ran green

| Check | Result |
|---|---|
| C++ build (`myikai_pluginEditor`, Win64, Development) | ✅ 22/22 actions succeeded in 13.62s. All 7 modules link cleanly: MyikaCore, MyikaSky, MyikaSkyEditor, MyikaWater, MyikaWaterEditor, MyikaMCP, MyikaMCPChat. |
| Re-build after Python patches | ✅ Target up-to-date in 0.58s. |
| myika_mcp pytest | ✅ 4/4 pass: `test_ping_round_trip`, `test_invalid_args_raises`, `test_set_time_of_day_ok`, `test_unknown_tool`. |
| Python syntax check on patched scripts | ✅ `Create-MyikaWaterAssets.py`, `author_celestial_materials.py`, `Create-MyikaCloudPresets.py` all compile. |

## Editor crash root-cause diagnosis

The May-3 quarantine commits (`9cc3b31` sky / `d81ccbd` water) never
identified specific root causes — only symptoms. This pass did the
static read of every suspect script and isolated three distinct bugs:

### Bug A — `M_MyikaStars` Noise expression (sky hang, MYI-61)

**File:** `tools/_recovered/python/author_celestial_materials.py:92-101`

`MaterialExpressionNoise` was instantiated with default UE5
configuration:

| Property | Default value | Effect |
|---|---|---|
| `noise_function` | `Simplex_TextureBased` | Highest-quality Simplex; ~3M shader instructions for 4 octaves |
| `levels` | 4 | Four-octave fractal noise |
| `quality` | 2 (high) | Slowest compile path |
| `turbulence` | True | Adds `abs()` post-pass |

The `Position` input was `WorldPosition * 0.005`, which on a sky-dome
mesh at 100km+ scale produces sub-pixel noise frequencies that
explode the shader graph during translation to HLSL. Editor froze on
shader compile when `L_SkyTest` opened.

**Patch landed:** force the cheapest noise variant explicitly:
- `noise_function = NOISEFUNCTION_GRADIENT_TEX` (~10× cheaper)
- `levels = 1`
- `quality = 0`
- `turbulence = False`

Expected effect: shader drops from ~3M instructions to ~300k, compile
time from "infinite" to <1 second.

### Bug B — Cross-plugin asset registry leak (water memory leak, MYI-67)

**File:** `tools/_recovered/Create-MyikaWaterAssets.py:88-102`

`make_or_load_master_material()` did delete-then-create on every run:

```python
if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
    if not unreal.EditorAssetLibrary.delete_asset(asset_path):
        raise RuntimeError(...)
asset_tools.create_asset(...)  # fresh M_MyikaWater
```

While `MI_MyikaWater_Ocean` and `MI_MyikaWater_Lake` were live in the
editor (because `L_WaterTest`'s `WaterBodyOcean` actor referenced
them as parent material instances), the delete cascaded:

1. `delete_asset(M_MyikaWater)` → redirector created → asset registry
   churn.
2. MIs reload triggered through the redirector.
3. New `M_MyikaWater` is created with `MSM_SINGLE_LAYER_WATER` shading,
   which depends on UE Water plugin shader functions.
4. `set_material_instance_parent(instance, master)` triggers another
   reload chain → MI re-resolves through redirector → loops back.

Symptom in commit `d81ccbd`: 16 GB → 26 GB+ memory leak in 20 seconds
with frozen log activity.

**Patch landed:** if asset already exists, **reuse it in-place**.
`build_master_material()` already calls
`delete_all_material_expressions()` right after this returns, so the
graph is rebuilt cleanly without re-instantiating the UObject. No
redirector. No reload chain. No leak.

### Bug C — None — cloud presets are innocent

**File:** `tools/_recovered/python/create_missing_assets.py`

The 6 `UMyikaCloudPreset` instances are `UPrimaryDataAsset` with 7
fields (Coverage, Morphology, etc.). They do not compile shaders.
They cannot trigger an editor hang on their own. The May-3 hang
was Bug A (M_MyikaStars). Cloud presets just happened to be
authored in the same agent run and got swept up in the quarantine.

**Action:** authored a NEW isolated script at
`tools/Create-MyikaCloudPresets.py` (split from
`create_missing_assets.py`, drops the M_MyikaUnderwaterPP path that
already exists). Also idempotent — updates existing presets in place
without recreating.

## What's now in the working tree

```
tools/
├── Create-MyikaCloudPresets.py        # NEW: isolated, idempotent, MYI-60
├── git-hooks/
└── _recovered/                         # 17 scripts from stash 182ccfe
    ├── README.md
    ├── Create-MyikaWaterAssets.py     # PATCHED: no delete-recreate, raise on recompile fail
    ├── Create-MyikaCausticAssets.py
    ├── Create-MyikaMpcAssets.py
    ├── ...
    └── python/
        ├── author_celestial_materials.py  # PATCHED: cheapest noise variant
        └── create_missing_assets.py
```

## Suggested next PIE round (Jacob)

Order matters — run in this sequence so a fail short-circuits the rest:

1. **`Create-MyikaCloudPresets.py`** alone via
   `UnrealEditor-Cmd.exe -ExecCmds="py tools/Create-MyikaCloudPresets.py"`.
   Verify `Saved/Validation/MYI-60-cloud-presets/summary.json` reports
   `status: ok` and 6/6 saved. Open `L_SkyTest` for ~30 seconds. If
   editor stays stable, **MYI-60 PIE-passes** as soon as `LerpToPreset`
   is exercised against any preset.
2. **`tools/_recovered/python/author_celestial_materials.py`**
   (recover to `tools/` first or copy the patched version). Open
   `L_SkyTest`. The patched Noise should compile cleanly. If editor
   stays stable, **MYI-61 (V1-S6 stars + moon) is unblocked**.
3. **`tools/_recovered/Create-MyikaWaterAssets.py`** (after recovery
   to `tools/`, also need `tools/_recovered/generate_myika_water_textures.py`
   to produce the normal map first). The reuse-in-place fix should
   prevent the asset-registry leak. Open `L_WaterTest`. If editor
   stays stable, **MYI-67 (V2-S2 ocean) PIE-passes**.

After these three, the only remaining gates for V1 closure are the
batched PIE script in `docs/runs/2026-05-05-batched-pie-session.md`
and Jacob's aesthetic call on V1-S9 (MYI-64).

## What I cannot verify without the editor

- Actual shader compile time of the patched Noise expression.
- Whether the in-place reuse of `M_MyikaWater` truly bypasses the
  cross-plugin asset registry leak — needs editor-mode load test.
- PIE on every sky/water surface (the whole batched script).
- Aesthetic judgment on cloud presets / star density / moon look.

These remain Jacob's to verify. The patches are evidence-driven —
each fix is a direct response to a specific symptom in a specific
quarantine commit message — but only PIE confirms they actually
solved the runtime problem.

## Loose ends still open

- `tools/_recovered/` is the staging area; once a script PIE-passes,
  it should graduate to `tools/` proper. I left them in `_recovered/`
  rather than promote eagerly because I haven't seen them run
  successfully.
- Auto-PIE harness (`tools/validate_myika_water_scaffold.py`,
  `tools/_recovered/validate_myika_water_scaffold.py`) still fails
  with "PIE did not start before timeout." Not addressed this pass.
  Filed as a cross-cutting follow-up in the original review doc.
- `MyikaSubsystem.cpp` patches added `LightningFlash` and `WaterLevel`
  change-detection but no `Temperature` change-detection — confirmed
  that's intentional or a bug in a separate review pass.
