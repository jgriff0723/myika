"""
Create the missing assets that V1-S5b cloud presets and V2-S6 underwater
post-process need but the original agent flood didn't author:

  1. 6 UMyikaCloudPreset UPrimaryDataAsset instances at
     /Myika/MyikaSky/Presets/Clouds/ (Clear/PartlyCloudy/Cumulus/Overcast/
     Storm/Alien) so AMyikaSkyActor.LerpToPreset has real targets.

  2. M_MyikaUnderwaterPP.uasset post-process material at
     /Myika/MyikaWater/Materials/ so UMyikaUnderwaterPostProcessComponent
     activates a real visible effect.

Run via:
    UnrealEditor-Cmd.exe <project> -nullrhi -unattended
        -ExecCmds="py <this-file>" -log -abslog=<log>
"""

import traceback
import unreal


PRESETS_FOLDER = "/Myika/MyikaSky/Presets/Clouds"
UNDERWATER_PP_FOLDER = "/Myika/MyikaWater/Materials"
UNDERWATER_PP_NAME = "M_MyikaUnderwaterPP"

# (asset_name, coverage, morphology, wind_strength, extinction, detail, wind_dir, base_color_rgba)
PRESETS = [
    ("Preset_Clear",        0.05, 0.30, 0.20, 0.40, 0.30, (1.0, 0.0, 0.0), (1.00, 1.00, 1.00, 1.00)),
    ("Preset_PartlyCloudy", 0.35, 0.50, 0.25, 0.45, 0.45, (1.0, 0.0, 0.0), (1.00, 1.00, 1.00, 1.00)),
    ("Preset_Cumulus",      0.55, 0.85, 0.20, 0.55, 0.65, (0.7, 0.3, 0.0), (1.00, 0.98, 0.96, 1.00)),
    ("Preset_Overcast",     0.85, 0.40, 0.35, 0.65, 0.30, (1.0, 0.0, 0.0), (0.78, 0.80, 0.85, 1.00)),
    ("Preset_Storm",        0.95, 0.30, 0.85, 0.85, 0.80, (1.0, 0.0, 0.0), (0.42, 0.45, 0.52, 1.00)),
    ("Preset_Alien",        0.65, 0.95, 0.40, 0.55, 0.90, (0.5, 0.5, 0.0), (0.55, 0.20, 0.85, 1.00)),
]


def log(msg):
    unreal.log(f"[create_missing_assets] {msg}")


def get_cloud_preset_class():
    return unreal.find_object(None, "/Script/MyikaSky.MyikaCloudPreset")


def create_preset(asset_name, coverage, morphology, wind_strength,
                  extinction, detail, wind_dir, base_color_rgba):
    full_path = f"{PRESETS_FOLDER}/{asset_name}"
    existing = unreal.load_asset(full_path)
    if existing is not None:
        log(f"{asset_name}: already exists, skipping")
        return True

    preset_class = get_cloud_preset_class()
    if preset_class is None:
        log(f"FAIL: UMyikaCloudPreset class not found in /Script/MyikaSky")
        return False

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", preset_class)
    new_asset = asset_tools.create_asset(asset_name, PRESETS_FOLDER, preset_class, factory)
    if new_asset is None:
        log(f"FAIL: create_asset returned None for {asset_name}")
        return False

    new_asset.set_editor_property("coverage", coverage)
    new_asset.set_editor_property("morphology", morphology)
    new_asset.set_editor_property("wind_strength", wind_strength)
    new_asset.set_editor_property("extinction", extinction)
    new_asset.set_editor_property("detail", detail)
    new_asset.set_editor_property("wind_direction",
                                  unreal.Vector(wind_dir[0], wind_dir[1], wind_dir[2]))
    new_asset.set_editor_property("base_color",
                                  unreal.LinearColor(*base_color_rgba))

    saved = unreal.EditorAssetLibrary.save_loaded_asset(new_asset, only_if_is_dirty=False)
    log(f"{asset_name}: created + saved={saved}")
    return saved


def create_underwater_pp():
    full_path = f"{UNDERWATER_PP_FOLDER}/{UNDERWATER_PP_NAME}"
    existing = unreal.load_asset(full_path)
    if existing is not None:
        log(f"{UNDERWATER_PP_NAME}: already exists, skipping")
        return True

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.MaterialFactoryNew()
    new_mat = asset_tools.create_asset(UNDERWATER_PP_NAME, UNDERWATER_PP_FOLDER,
                                       unreal.Material, factory)
    if new_mat is None:
        log("FAIL: M_MyikaUnderwaterPP create_asset returned None")
        return False

    try:
        new_mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
        new_mat.set_editor_property("blendable_location",
                                    unreal.BlendableLocation.BL_BEFORE_TRANSLUCENCY)
    except Exception as e:
        log(f"WARN setting underwater material domain: {e}")

    lib = unreal.MaterialEditingLibrary

    # SceneTexture sample (PostProcessInput0 = source scene color).
    scene_tex = lib.create_material_expression(
        new_mat, unreal.MaterialExpressionSceneTexture, -600, 0
    )
    try:
        scene_tex.set_editor_property("scene_texture_id",
                                      unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
    except Exception as e:
        log(f"WARN scene_texture_id set: {e}")

    # Underwater blue tint constant.
    tint = lib.create_material_expression(
        new_mat, unreal.MaterialExpressionConstant3Vector, -600, 200
    )
    tint.set_editor_property("constant",
                             unreal.LinearColor(0.20, 0.45, 0.70, 1.0))

    # Lerp scene color toward tint by a blend factor.
    blend_amount = lib.create_material_expression(
        new_mat, unreal.MaterialExpressionConstant, -600, 360
    )
    blend_amount.set_editor_property("r", 0.55)

    lerp = lib.create_material_expression(
        new_mat, unreal.MaterialExpressionLinearInterpolate, -300, 100
    )
    lib.connect_material_expressions(scene_tex, "Color", lerp, "A")
    lib.connect_material_expressions(tint, "", lerp, "B")
    lib.connect_material_expressions(blend_amount, "", lerp, "Alpha")

    lib.connect_material_property(lerp, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    # Force fully opaque post-process output.
    one = lib.create_material_expression(
        new_mat, unreal.MaterialExpressionConstant, -300, 320
    )
    one.set_editor_property("r", 1.0)
    lib.connect_material_property(one, "", unreal.MaterialProperty.MP_OPACITY)

    lib.recompile_material(new_mat)
    saved = unreal.EditorAssetLibrary.save_loaded_asset(new_mat, only_if_is_dirty=False)
    log(f"{UNDERWATER_PP_NAME} created + saved={saved}")
    return saved


def main():
    log("=== START ===")
    preset_results = []
    for spec in PRESETS:
        try:
            ok = create_preset(*spec)
        except Exception as e:
            log(f"EXCEPTION on {spec[0]}: {e}\n{traceback.format_exc()}")
            ok = False
        preset_results.append((spec[0], ok))

    try:
        underwater_ok = create_underwater_pp()
    except Exception as e:
        log(f"EXCEPTION underwater: {e}\n{traceback.format_exc()}")
        underwater_ok = False

    log(f"=== DONE presets={preset_results} underwater={underwater_ok} ===")


main()

try:
    unreal.SystemLibrary.quit_editor()
except Exception:
    import sys
    sys.exit(0)
