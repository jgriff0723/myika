"""
Author the V1-S4 curve atlas for AMyikaSkyActor 24h sky lighting.

Creates 6 curve assets at /Myika/MyikaSky/Curves/:

- C_MyikaSunColor       (UCurveLinearColor) - sun light color over 24h
- C_MyikaSkyLuminance   (UCurveLinearColor) - sky tint over 24h
- C_MyikaFogColor       (UCurveLinearColor) - height-fog inscatter over 24h
- C_MyikaSunIntensity   (UCurveFloat)       - sun intensity in lux 0..150000
- C_MyikaFogDensity     (UCurveFloat)       - exponential height-fog density
- C_MyikaAutoExposure   (UCurveFloat)       - PP exposure bias

Key points keyed at sensible hours (0/4/6/8/12/16/18/20/22/24) so the sky
runs through dawn -> golden hour -> noon -> dusk -> night smoothly.

Run via:
    UnrealEditor-Cmd.exe <project> -nullrhi -unattended
        -ExecCmds="py <this-file>" -log -abslog=<log>
"""

import traceback
import unreal


CURVES_FOLDER = "/Myika/MyikaSky/Curves"


def log(msg):
    unreal.log(f"[author_sky_curves] {msg}")


# Hour-keyed colour curves: list of (time_hours, LinearColor(r,g,b,a))
SUN_COLOR_KEYS = [
    (0.0,  unreal.LinearColor(0.05, 0.07, 0.12, 1.0)),
    (4.0,  unreal.LinearColor(0.10, 0.10, 0.18, 1.0)),
    (6.0,  unreal.LinearColor(1.00, 0.55, 0.30, 1.0)),
    (8.0,  unreal.LinearColor(1.00, 0.92, 0.80, 1.0)),
    (12.0, unreal.LinearColor(1.00, 0.98, 0.95, 1.0)),
    (16.0, unreal.LinearColor(1.00, 0.92, 0.80, 1.0)),
    (18.0, unreal.LinearColor(1.00, 0.50, 0.22, 1.0)),
    (20.0, unreal.LinearColor(0.40, 0.20, 0.25, 1.0)),
    (22.0, unreal.LinearColor(0.10, 0.10, 0.18, 1.0)),
    (24.0, unreal.LinearColor(0.05, 0.07, 0.12, 1.0)),
]

SKY_LUMINANCE_KEYS = [
    (0.0,  unreal.LinearColor(0.02, 0.03, 0.06, 1.0)),
    (4.0,  unreal.LinearColor(0.05, 0.06, 0.14, 1.0)),
    (6.0,  unreal.LinearColor(0.55, 0.40, 0.45, 1.0)),
    (8.0,  unreal.LinearColor(0.40, 0.55, 0.85, 1.0)),
    (12.0, unreal.LinearColor(0.55, 0.75, 1.00, 1.0)),
    (16.0, unreal.LinearColor(0.45, 0.60, 0.90, 1.0)),
    (18.0, unreal.LinearColor(0.85, 0.40, 0.35, 1.0)),
    (20.0, unreal.LinearColor(0.30, 0.18, 0.30, 1.0)),
    (22.0, unreal.LinearColor(0.05, 0.06, 0.14, 1.0)),
    (24.0, unreal.LinearColor(0.02, 0.03, 0.06, 1.0)),
]

FOG_COLOR_KEYS = [
    (0.0,  unreal.LinearColor(0.05, 0.07, 0.10, 1.0)),
    (6.0,  unreal.LinearColor(0.85, 0.55, 0.45, 1.0)),
    (8.0,  unreal.LinearColor(0.70, 0.78, 0.92, 1.0)),
    (12.0, unreal.LinearColor(0.70, 0.85, 1.00, 1.0)),
    (16.0, unreal.LinearColor(0.78, 0.80, 0.95, 1.0)),
    (18.0, unreal.LinearColor(0.95, 0.60, 0.40, 1.0)),
    (20.0, unreal.LinearColor(0.30, 0.18, 0.30, 1.0)),
    (24.0, unreal.LinearColor(0.05, 0.07, 0.10, 1.0)),
]

# Hour-keyed scalar curves: list of (time_hours, value)
SUN_INTENSITY_KEYS = [
    (0.0,    0.0),
    (5.0,    100.0),
    (6.0,    5000.0),
    (8.0,    55000.0),
    (12.0,   110000.0),
    (16.0,   80000.0),
    (18.0,   8000.0),
    (19.0,   200.0),
    (20.0,   0.0),
    (24.0,   0.0),
]

FOG_DENSITY_KEYS = [
    (0.0,   0.045),
    (6.0,   0.025),
    (8.0,   0.018),
    (12.0,  0.013),
    (16.0,  0.018),
    (18.0,  0.030),
    (22.0,  0.045),
    (24.0,  0.045),
]

AUTO_EXPOSURE_KEYS = [
    (0.0,   1.6),
    (6.0,   0.7),
    (8.0,   0.2),
    (12.0,  0.0),
    (16.0,  0.2),
    (18.0,  0.6),
    (22.0,  1.6),
    (24.0,  1.6),
]


def get_or_create_curve(asset_name, asset_class, factory_class):
    full_path = f"{CURVES_FOLDER}/{asset_name}"
    existing = unreal.load_asset(full_path)
    if existing is not None:
        if isinstance(existing, asset_class):
            log(f"{asset_name}: already exists, will overwrite keys")
            return existing
        log(f"WARN {asset_name} exists as {type(existing).__name__}, expected {asset_class.__name__}; skipping")
        return None

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = factory_class()
    new_asset = asset_tools.create_asset(asset_name, CURVES_FOLDER, asset_class, factory)
    if new_asset is None:
        log(f"FAIL: create_asset returned None for {asset_name}")
    else:
        log(f"created {full_path}")
    return new_asset


def write_color_curve(asset_name, keys):
    curve = get_or_create_curve(asset_name, unreal.CurveLinearColor,
                                unreal.CurveLinearColorFactory)
    if curve is None:
        return False

    # Try several known Python attribute names for the underlying RichCurves.
    # UE 5.7 hides FloatCurves[] via standard property access but the asset
    # often exposes it as a python attribute or via getattr.
    channels = None
    for attr_name in ("float_curves", "FloatCurves"):
        try:
            channels = getattr(curve, attr_name)
            if channels is not None:
                log(f"  using attribute {attr_name}")
                break
        except Exception:
            continue

    if channels is None:
        # Last resort: write a console exec to set keys via the editor.
        # Can't author -- bail with empty curve.
        log(f"{asset_name}: NO writable channel attribute; created empty (will need manual keys)")
        unreal.EditorAssetLibrary.save_loaded_asset(curve, only_if_is_dirty=False)
        return False

    for ch in channels:
        try:
            ch.keys = []
        except Exception:
            pass
    for (t, color) in keys:
        components = (color.r, color.g, color.b, color.a)
        for idx, ch in enumerate(channels):
            ch.add_key(t, components[idx])

    saved = unreal.EditorAssetLibrary.save_loaded_asset(curve, only_if_is_dirty=False)
    log(f"{asset_name}: wrote {len(keys)} keys, saved={saved}")
    return saved


def write_float_curve(asset_name, keys):
    curve = get_or_create_curve(asset_name, unreal.CurveFloat,
                                unreal.CurveFloatFactory)
    if curve is None:
        return False

    float_curve = None
    for attr_name in ("float_curve", "FloatCurve"):
        try:
            float_curve = getattr(curve, attr_name)
            if float_curve is not None:
                log(f"  using attribute {attr_name}")
                break
        except Exception:
            continue

    if float_curve is None:
        log(f"{asset_name}: NO writable float_curve attribute; created empty")
        unreal.EditorAssetLibrary.save_loaded_asset(curve, only_if_is_dirty=False)
        return False

    try:
        float_curve.keys = []
    except Exception:
        pass
    for (t, v) in keys:
        float_curve.add_key(t, v)

    saved = unreal.EditorAssetLibrary.save_loaded_asset(curve, only_if_is_dirty=False)
    log(f"{asset_name}: wrote {len(keys)} keys, saved={saved}")
    return saved


def main():
    log("=== START ===")
    results = {}
    try:
        results["C_MyikaSunColor"]     = write_color_curve("C_MyikaSunColor", SUN_COLOR_KEYS)
        results["C_MyikaSkyLuminance"] = write_color_curve("C_MyikaSkyLuminance", SKY_LUMINANCE_KEYS)
        results["C_MyikaFogColor"]     = write_color_curve("C_MyikaFogColor", FOG_COLOR_KEYS)
        results["C_MyikaSunIntensity"] = write_float_curve("C_MyikaSunIntensity", SUN_INTENSITY_KEYS)
        results["C_MyikaFogDensity"]   = write_float_curve("C_MyikaFogDensity", FOG_DENSITY_KEYS)
        results["C_MyikaAutoExposure"] = write_float_curve("C_MyikaAutoExposure", AUTO_EXPOSURE_KEYS)
    except Exception as e:
        log(f"EXCEPTION: {e}\n{traceback.format_exc()}")
    log(f"=== DONE results={results} ===")


main()

try:
    unreal.SystemLibrary.quit_editor()
except Exception:
    import sys
    sys.exit(0)
