# ADR-0004: Blueprint slider discipline (artist-facing config)

**Status:** Accepted
**Date:** 2026-05-03
**Deciders:** Jacob Griffith (human), Claude (Myika)

## Context

Per ADR-0001, the Myika plugin ships systems that artists drop into scenes and tune. Jacob's mental model is "I drag `BP_MyikaSky` into the level, pick a cloud preset from a dropdown, scrub StormIntensity with a slider, and it just works." Without an explicit discipline, agents (Neco) will write functional but artist-hostile code: hidden parameters, no clamps, no categories, magic numbers in C++.

This ADR locks the **mandatory pattern** every Myika BP/component/actor/data-asset must follow.

## Decision

### Rule 1 — Every artist-tunable is a UPROPERTY with these specifiers

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky")
float StormIntensity = 0.f;
```

Required: `EditAnywhere`, `BlueprintReadWrite`, `Category` namespaced as `Myika|<System>`.
Forbidden: `EditDefaultsOnly` (artists must edit instances), `BlueprintReadOnly` on tunables (UI must drive them).

### Rule 2 — Float sliders use `meta=()` clamps + UI hints

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky",
          meta=(ClampMin="0.0", ClampMax="1.0",
                UIMin="0.0",    UIMax="1.0",
                SliderExponent="2.0"))
float StormIntensity = 0.f;
```

- `ClampMin`/`ClampMax` — hard limits (engine refuses out-of-range)
- `UIMin`/`UIMax` — slider visual range (can equal clamps)
- `SliderExponent="2.0"` — non-linear feel for 0..1 perceptual params (clouds, fog, wetness)
- For HDR / large-range values: set realistic `UIMax` (e.g. sun intensity `UIMax="150000.0"` lumens) but no `ClampMax` if any positive value is valid

### Rule 3 — Group params by category for clean Details panel

Categories use `|` delimiter for sub-grouping:

| Category | Use for |
|----------|---------|
| `Myika\|Sky` | Sky controller params |
| `Myika\|Sky\|Sun` | Sun-specific (rotation, color, intensity) |
| `Myika\|Sky\|Clouds` | Cloud-specific (preset, coverage, wind) |
| `Myika\|Sky\|Stars` | Stars/moon visibility |
| `Myika\|Weather` | Weather state, type, transitions |
| `Myika\|Water` | Water body shared params |
| `Myika\|Water\|Surface` | Surface material params |
| `Myika\|Water\|Underwater` | Underwater PP params |
| `Myika\|Landscape` | Landscape master params |
| `Myika\|PCG` | PCG biome params |
| `Myika\|NPC` | NPC behavior, perception, combat |
| `Myika\|Items` | Inventory, crafting, vendor |
| `Myika\|Debug` | Debug overrides (force time, force weather) |

Subcategories collapse cleanly in the Details panel. Artists never see uncategorized params.

### Rule 4 — One drop-in actor per system, defaults render correctly

Each system ships **exactly one** placeable Actor that an artist can drop with zero setup:

| System | Drop-in actor |
|--------|---------------|
| Sky | `AMyikaSky` — placing it spawns sun + sky + clouds + stars + lightning ready to go |
| Weather | `AMyikaWeather` (V5) — single biome volume; default = clear |
| Water Ocean | `AMyikaOcean` — drop, get a working ocean |
| Water River | `AMyikaRiver` — drop, draw spline, river flows |
| Water Lake | `AMyikaLake` — drop, define spline boundary, lake renders |
| PCG Biome | `AMyikaBiomeRegion` (V4) — drop volume, biome scatters |
| NPC Population | `AMyikaPopulationManager` (V10) — drop region, NPCs spawn |

**Defaults must render correctly without any further config.** If the artist has to set 3 things to make it look not-broken, the defaults are wrong.

### Rule 5 — Editor Utility Widgets for live tuning (`EUW_*`)

Every system ships an `EUW_<System>Debug` widget (e.g. `EUW_SkyDebug`, `EUW_WaterDebug`) with:
- Sliders for the top 5-10 most-tuned params
- Preset buttons where presets exist
- Live debug labels (current ToD, current weather, sun altitude, etc.)
- Reset-to-defaults button

These let artists tune in PIE without rebuilding or jumping back to the Details panel.

### Rule 6 — Per-instance overrides via Blueprint, not C++ subclassing

Artists should never need to make a C++ subclass for tuning. Everything tunable is exposed via the BP class defaults panel + per-instance overrides in the level. C++ subclasses are reserved for behavior changes, not parameter changes.

### Rule 7 — Data Asset presets where parameter sets cluster

When a system has named parameter presets (e.g. cloud morphology, biome variants, weather types), use `UPrimaryDataAsset` subclasses (e.g. `UMyikaCloudPreset`, `UMyikaBiomePreset`). Reasons:
- Artists author presets in the Content Browser, not in C++
- Hot-reload friendly (no recompile to add a preset)
- Versionable in source control as separate `.uasset` files
- Easy to ship as content packs

### Rule 8 — Blueprint-callable functions for runtime control

Anything the artist might call from a Sequencer track, level Blueprint, or gameplay event must be `UFUNCTION(BlueprintCallable)`:

```cpp
UFUNCTION(BlueprintCallable, Category = "Myika|Sky|Weather",
          meta=(DisplayName = "Lerp To Cloud Preset"))
void LerpToPreset(UMyikaCloudPreset* TargetPreset, float DurationSeconds);
```

`DisplayName` overrides the auto-generated name when needed for clarity.

### Rule 9 — Minimum required UPROPERTY metadata for common types

| Type | Required meta |
|------|---------------|
| `float` slider | `ClampMin`, `ClampMax`, `UIMin`, `UIMax` (often `SliderExponent`) |
| `int32` slider | `ClampMin`, `ClampMax`, `UIMin`, `UIMax` |
| `FVector` direction | `meta=(MakeEditWidget=true)` for in-viewport gizmo where useful |
| `TObjectPtr<U...>` asset | `meta=(AllowedClasses="UMyikaCloudPreset")` to filter content browser |
| `enum` | nothing extra; UE makes it a dropdown automatically |
| `FLinearColor` | nothing extra; UE makes it a color picker automatically |
| `FName` for tag | `meta=(GameplayTagFilter="...")` for gameplay-tag-typed FNames |

### Rule 10 — Comment every UPROPERTY with a one-line tooltip

UE picks up `///` comments above UPROPERTYs as Details-panel tooltips:

```cpp
/** Storm intensity 0..1. Drives cloud darkness, lightning frequency, wind strength. */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Myika|Sky",
          meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
float StormIntensity = 0.f;
```

Artists hover the param name in the Details panel and see the tooltip — eliminates "what does this do?" Slack questions.

## Consequences

**Positive:**
- Artist-friendly from day one — no second pass to "make it tunable"
- Consistent Details panel UX across all 11 verticals (Sky, Water, Landscape, PCG, Weather, Interaction, Swap, Items, Swim, NPC, NiagaraUI)
- Sequencer / level Blueprint integration is automatic (Rule 8)
- Presets ship as content (Rule 7) — easy DLC / content packs

**Negative:**
- Slightly more verbose UPROPERTY declarations
- Agents (Neco) need to internalize this; first slice may have to be re-edited if missed
- Tooltip writing adds 30s per param

**Neutral:**
- Matches UE community best practices already; just locked + enforced

## Validation

Each slice's PR/run-log must demonstrate:
- [ ] Drop-in actor renders correctly with zero setup (Rule 4)
- [ ] All artist-tunable params have `Category="Myika|*"` (Rule 1)
- [ ] All float sliders have clamps + UI hints (Rule 2)
- [ ] `EUW_*Debug` widget exists for the system (Rule 5)
- [ ] Tooltips on every UPROPERTY (Rule 10)

## Enforcement

Code reviewer / `superpowers:code-reviewer` agent should reject PRs that miss Rules 1-10.

## References

- ADR-0001 (operating model — context for "artist-facing")
- `Plugins/Myika/Source/MyikaCore/Public/MyikaTypes.h` — first reference implementation; extend the same pattern across new code
- `Plugins/Myika/Content/ASSET-ONBOARDING.md` — companion SOP for asset placement (ADR-0005 follow-up)
