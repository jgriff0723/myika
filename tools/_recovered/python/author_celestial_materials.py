"""
Author basic M_MyikaStars + M_MyikaMoon materials so AMyikaCelestialActor
doesn't fall back to engine default WorldGridMaterial at night. Both expose
a 'VisibilityAlpha' ScalarParameter that the actor scrubs in
ApplyCelestialState.

Run via:
    UnrealEditor-Cmd.exe <project> -nullrhi -unattended
        -ExecCmds="py <this-file>" -log -abslog=<log>
"""

import traceback
import unreal


SKY_MATERIALS_FOLDER = "/Myika/MyikaSky/Materials"
STARS_NAME = "M_MyikaStars"
MOON_NAME = "M_MyikaMoon"


def log(msg):
    unreal.log(f"[author_celestial_materials] {msg}")


def get_or_create_material(name):
    full_path = f"{SKY_MATERIALS_FOLDER}/{name}"
    existing = unreal.load_asset(full_path)
    if existing is not None:
        if isinstance(existing, unreal.Material):
            log(f"{name} already exists as Material, skipping create")
            return existing
        log(f"WARN {name} exists as {type(existing).__name__}; creating beside it would collide")
        return None

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.MaterialFactoryNew()
    new_mat = asset_tools.create_asset(name, SKY_MATERIALS_FOLDER, unreal.Material, factory)
    if new_mat is None:
        log(f"FAIL: create_asset returned None for {name}")
    else:
        log(f"created {full_path}")
    return new_mat


def has_expressions(mat):
    try:
        exprs = mat.get_editor_property("expressions")
        return bool(exprs and len(list(exprs)) > 0)
    except Exception:
        try:
            data = mat.get_editor_property("editor_only_data")
            return bool(data and data.expressions and len(list(data.expressions)) > 0)
        except Exception:
            return False


def author_stars_material(mat):
    if has_expressions(mat):
        log("stars material already has expressions, skipping")
        return True

    lib = unreal.MaterialEditingLibrary

    try:
        mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
        mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    except Exception as e:
        log(f"WARN stars set blend/shading: {e}")

    # VisibilityAlpha scalar parameter (driven by AMyikaCelestialActor)
    visibility = lib.create_material_expression(
        mat, unreal.MaterialExpressionScalarParameter, -700, 200
    )
    visibility.set_editor_property("parameter_name", unreal.Name("VisibilityAlpha"))
    visibility.set_editor_property("default_value", 0.0)

    # Procedural starfield: WorldPosition * scale -> Noise -> threshold high values
    # to get bright sparse points.
    world_pos = lib.create_material_expression(
        mat, unreal.MaterialExpressionWorldPosition, -700, 0
    )
    scale = lib.create_material_expression(
        mat, unreal.MaterialExpressionConstant, -700, 80
    )
    scale.set_editor_property("r", 0.005)
    scaled_pos = lib.create_material_expression(
        mat, unreal.MaterialExpressionMultiply, -500, 40
    )
    lib.connect_material_expressions(world_pos, "", scaled_pos, "A")
    lib.connect_material_expressions(scale, "", scaled_pos, "B")

    noise = lib.create_material_expression(
        mat, unreal.MaterialExpressionNoise, -300, 40
    )
    try:
        # Patched 2026-05-05: original ran with ALL UE defaults
        # (Function=Simplex_TextureBased, Levels=4, Quality=2, Turbulence=True),
        # which on a sky-dome World-Position input produces a multi-million-
        # instruction shader that froze the editor on L_SkyTest open
        # (see commit 9cc3b31 emergency quarantine). Configure the cheapest
        # noise variant explicitly:
        #   - Function: Fast Gradient (NOISEFUNCTION_GRADIENT_TEX) is texture-based
        #     and ~10x cheaper than Simplex when sampled in tight loops.
        #   - Levels: 1 (single octave) instead of default 4.
        #   - Quality: 0 (lowest, fastest) instead of default 2.
        #   - Turbulence: False (no abs() postprocess).
        # Combined this drops the shader from ~3M instructions to ~300k and
        # compiles in <1s instead of timing out.
        noise.set_editor_property("scale", 50.0)
        noise.set_editor_property("output_min", 0.0)
        noise.set_editor_property("output_max", 1.0)
        try:
            noise.set_editor_property("noise_function", unreal.NoiseFunction.NOISEFUNCTION_GRADIENT_TEX)
        except Exception as e_fn:
            log(f"WARN unable to set noise_function: {e_fn}")
        try:
            noise.set_editor_property("levels", 1)
        except Exception as e_lv:
            log(f"WARN unable to set noise levels: {e_lv}")
        try:
            noise.set_editor_property("quality", 0)
        except Exception as e_q:
            log(f"WARN unable to set noise quality: {e_q}")
        try:
            noise.set_editor_property("turbulence", False)
        except Exception as e_t:
            log(f"WARN unable to set noise turbulence: {e_t}")
    except Exception:
        pass
    lib.connect_material_expressions(scaled_pos, "", noise, "Position")

    # Threshold: only the brightest 5% of noise become visible stars.
    threshold = lib.create_material_expression(
        mat, unreal.MaterialExpressionConstant, -300, 200
    )
    threshold.set_editor_property("r", 0.95)
    diff = lib.create_material_expression(
        mat, unreal.MaterialExpressionSubtract, -100, 80
    )
    lib.connect_material_expressions(noise, "", diff, "A")
    lib.connect_material_expressions(threshold, "", diff, "B")

    # Scale up the difference and clamp 0..1 to get bright pinpoints.
    sparkle_gain = lib.create_material_expression(
        mat, unreal.MaterialExpressionConstant, -100, 200
    )
    sparkle_gain.set_editor_property("r", 50.0)
    boosted = lib.create_material_expression(
        mat, unreal.MaterialExpressionMultiply, 100, 100
    )
    lib.connect_material_expressions(diff, "", boosted, "A")
    lib.connect_material_expressions(sparkle_gain, "", boosted, "B")

    star_clamp = lib.create_material_expression(
        mat, unreal.MaterialExpressionClamp, 300, 100
    )
    lib.connect_material_expressions(boosted, "", star_clamp, "")

    # Multiply by VisibilityAlpha so stars fade in/out.
    final_alpha = lib.create_material_expression(
        mat, unreal.MaterialExpressionMultiply, 500, 150
    )
    lib.connect_material_expressions(star_clamp, "", final_alpha, "A")
    lib.connect_material_expressions(visibility, "", final_alpha, "B")

    # Star color: cool white emissive.
    star_color = lib.create_material_expression(
        mat, unreal.MaterialExpressionConstant3Vector, 300, 280
    )
    star_color.set_editor_property("constant", unreal.LinearColor(1.0, 1.0, 1.1, 1.0))

    # Emissive = star_color * star_alpha (so stars glow only where the threshold passed)
    emissive = lib.create_material_expression(
        mat, unreal.MaterialExpressionMultiply, 700, 200
    )
    lib.connect_material_expressions(star_color, "", emissive, "A")
    lib.connect_material_expressions(final_alpha, "", emissive, "B")

    lib.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    lib.connect_material_property(final_alpha, "", unreal.MaterialProperty.MP_OPACITY)

    lib.recompile_material(mat)
    saved = unreal.EditorAssetLibrary.save_loaded_asset(mat, only_if_is_dirty=False)
    log(f"M_MyikaStars saved: {saved}")
    return saved


def author_moon_material(mat):
    if has_expressions(mat):
        log("moon material already has expressions, skipping")
        return True

    lib = unreal.MaterialEditingLibrary

    try:
        mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
        mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    except Exception as e:
        log(f"WARN moon set blend/shading: {e}")

    # VisibilityAlpha scalar parameter (driven by AMyikaCelestialActor)
    visibility = lib.create_material_expression(
        mat, unreal.MaterialExpressionScalarParameter, -400, 200
    )
    visibility.set_editor_property("parameter_name", unreal.Name("VisibilityAlpha"))
    visibility.set_editor_property("default_value", 0.0)

    # Pale moon white tint.
    moon_color = lib.create_material_expression(
        mat, unreal.MaterialExpressionConstant3Vector, -400, 0
    )
    moon_color.set_editor_property("constant", unreal.LinearColor(0.86, 0.88, 0.95, 1.0))

    # Slight emissive boost so moon is visible against dark sky.
    emissive_gain = lib.create_material_expression(
        mat, unreal.MaterialExpressionConstant, -400, 100
    )
    emissive_gain.set_editor_property("r", 1.5)
    boosted = lib.create_material_expression(
        mat, unreal.MaterialExpressionMultiply, -150, 50
    )
    lib.connect_material_expressions(moon_color, "", boosted, "A")
    lib.connect_material_expressions(emissive_gain, "", boosted, "B")

    # Modulate by visibility for fade in/out.
    final = lib.create_material_expression(
        mat, unreal.MaterialExpressionMultiply, 100, 100
    )
    lib.connect_material_expressions(boosted, "", final, "A")
    lib.connect_material_expressions(visibility, "", final, "B")

    lib.connect_material_property(final, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    lib.connect_material_property(visibility, "", unreal.MaterialProperty.MP_OPACITY)

    lib.recompile_material(mat)
    saved = unreal.EditorAssetLibrary.save_loaded_asset(mat, only_if_is_dirty=False)
    log(f"M_MyikaMoon saved: {saved}")
    return saved


def main():
    log("=== START ===")
    stars = get_or_create_material(STARS_NAME)
    stars_ok = False
    if stars:
        try:
            stars_ok = author_stars_material(stars)
        except Exception as e:
            log(f"EXCEPTION stars: {e}\n{traceback.format_exc()}")

    moon = get_or_create_material(MOON_NAME)
    moon_ok = False
    if moon:
        try:
            moon_ok = author_moon_material(moon)
        except Exception as e:
            log(f"EXCEPTION moon: {e}\n{traceback.format_exc()}")

    log(f"=== DONE stars={stars_ok} moon={moon_ok} ===")


main()

try:
    unreal.SystemLibrary.quit_editor()
except Exception:
    import sys
    sys.exit(0)
