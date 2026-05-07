# MyikaWater - Asset Registry

This file tracks the source, license, and current location of every non-trivial
asset shipped from this folder. Keep it updated before any Niagara, material,
render-target, or map asset gets committed.

## Planned assets

- `NS_MyikaShorelineRipple` - duplicated from the UE Niagara Fluids `Grid2D Shallow Water` template under UE engine content; runtime shoreline ripple solver for `UMyikaShorelineRippleComponent`
- `M_MyikaWater` / `MI_MyikaWater_Ocean` / `MI_MyikaWater_Lake` - Myika-authored Single Layer Water materials for V2-S2 and V2-S5
- `RT_MyikaShorelineRipple` - Myika-authored render target output used by shoreline ripples

## Procedural caustics - added 2026-05-03

**Source:** Generated locally by `tools/generate_myika_caustic_source.py`
**License:** MIT
**Attribution required:** none
**Intended use:** 4x4 caustic atlas and deferred decal material for `AMyikaCausticDecalActor`
**Files:**
- `tools/Generated/MyikaWater/T_MyikaCaustic_Flipbook.png` -> `/Myika/MyikaWater/Decals/T_MyikaCaustic_Flipbook`
- `tools/Create-MyikaCausticAssets.py` -> `/Myika/MyikaWater/Decals/M_MyikaCaustic`

## Procedural water normal - added 2026-05-03

**Source:** Generated locally by `tools/generate_myika_water_textures.py`
**License:** MIT
**Attribution required:** none
**Intended use:** Tiling normal map input for `M_MyikaWater` and the `MI_MyikaWater_{Ocean,Lake}` material instances
**Files:**
- `tools/Generated/MyikaWater/T_MyikaWater_Normal.png` -> `/Myika/MyikaWater/Materials/T_MyikaWater_Normal`
- `tools/Create-MyikaWaterAssets.py` -> `/Myika/MyikaWater/Materials/M_MyikaWater`
