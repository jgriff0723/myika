"""
Author minimum-viable shader graphs for the Myika water stack.

Discovery from the v2 run: M_MyikaWater.uasset is a MaterialInstanceConstant
(not a Material), so we can't author a graph on it. The fix is:
  1. Create a NEW Material 'M_MyikaWaterMaster' with the SLW graph.
  2. Re-parent existing MI_MyikaWater_Ocean and MI_MyikaWater_Lake to it.
  3. Author the M_MyikaCaustic deferred-decal graph (already a real Material).

Idempotent: skips creation if the target Material already exists with content.

Run via:
    UnrealEditor-Cmd.exe <project> -nullrhi -unattended
        -ExecCmds="py <this-file>;quit" -log -abslog=<log>
"""

import traceback
import unreal


WATER_MASTER_PATH = "/Myika/MyikaWater/Materials/M_MyikaWaterMaster"
OCEAN_INSTANCE_PATH = "/Myika/MyikaWater/Materials/MI_MyikaWater_Ocean"
LAKE_INSTANCE_PATH = "/Myika/MyikaWater/Materials/MI_MyikaWater_Lake"
CAUSTIC_MATERIAL_PATH = "/Myika/MyikaWater/Decals/M_MyikaCaustic"

WATER_MASTER_FOLDER = "/Myika/MyikaWater/Materials"
WATER_MASTER_NAME = "M_MyikaWaterMaster"


def log(msg):
    unreal.log(f"[author_water_materials] {msg}")


def _has_expressions(mat):
    try:
        exprs = mat.get_editor_property("expressions")
        return bool(exprs and len(list(exprs)) > 0)
    except Exception:
        try:
            data = mat.get_editor_property("editor_only_data")
            return bool(data and data.expressions and len(list(data.expressions)) > 0)
        except Exception:
            return False


def create_or_get_master_material():
    """Return the M_MyikaWaterMaster Material, creating it if missing."""
    existing = unreal.load_asset(WATER_MASTER_PATH)
    if existing is not None:
        if isinstance(existing, unreal.Material):
            log(f"master exists at {WATER_MASTER_PATH}")
            return existing
        log(f"WARN: {WATER_MASTER_PATH} exists but is {type(existing).__name__}, not Material")
        return None

    log(f"creating new Material at {WATER_MASTER_PATH}")
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.MaterialFactoryNew()
    new_mat = asset_tools.create_asset(WATER_MASTER_NAME, WATER_MASTER_FOLDER, unreal.Material, factory)
    if new_mat is None:
        log("FAIL: create_asset returned None")
        return None
    log(f"created {WATER_MASTER_PATH}")
    return new_mat


def author_master_graph(mat):
    """Wire a minimum-viable Single Layer Water graph onto mat."""
    if _has_expressions(mat):
        log("master already has expressions; skipping graph author")
        return True

    lib = unreal.MaterialEditingLibrary

    try:
        mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_SINGLE_LAYER_WATER)
        mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
        log("set shading model + blend mode")
    except Exception as e:
        log(f"WARN setting shading_model/blend_mode: {e}")

    log("creating BaseColor (deep ocean blue)...")
    base = lib.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, 0)
    base.set_editor_property("constant", unreal.LinearColor(0.012, 0.085, 0.135, 1.0))
    lib.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)

    log("creating Metallic=0...")
    metal = lib.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 120)
    metal.set_editor_property("r", 0.0)
    lib.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)

    log("creating Roughness=0.05...")
    rough = lib.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 200)
    rough.set_editor_property("r", 0.05)
    lib.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

    log("creating Specular=0.5...")
    spec = lib.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 280)
    spec.set_editor_property("r", 0.5)
    lib.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)

    log("creating Normal=(0,0,1)...")
    normal = lib.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, 380)
    normal.set_editor_property("constant", unreal.LinearColor(0.0, 0.0, 1.0, 1.0))
    lib.connect_material_property(normal, "", unreal.MaterialProperty.MP_NORMAL)

    log("creating SubsurfaceColor (turquoise)...")
    sss = lib.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, 480)
    sss.set_editor_property("constant", unreal.LinearColor(0.04, 0.18, 0.22, 1.0))
    lib.connect_material_property(sss, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)

    log("recompiling master...")
    lib.recompile_material(mat)
    log("saving master...")
    saved = unreal.EditorAssetLibrary.save_asset(WATER_MASTER_PATH, only_if_is_dirty=False)
    log(f"master saved: {saved}")
    return True


def reparent_instance(instance_path, new_parent):
    """Set MIC's parent to new_parent and save."""
    mic = unreal.load_asset(instance_path)
    if mic is None:
        log(f"WARN: instance {instance_path} not found, skipping")
        return False
    if not isinstance(mic, unreal.MaterialInstanceConstant):
        log(f"WARN: {instance_path} is {type(mic).__name__}, not MIC")
        return False
    current_parent = mic.get_editor_property("parent")
    current_name = current_parent.get_path_name() if current_parent else "<none>"
    log(f"{instance_path}: parent currently = {current_name}")
    if current_parent and current_parent.get_path_name() == new_parent.get_path_name():
        log("  already correct, skipping reparent")
        return True
    mic.set_editor_property("parent", new_parent)
    saved = unreal.EditorAssetLibrary.save_asset(instance_path, only_if_is_dirty=False)
    log(f"  reparented + saved: {saved}")
    return True


def fix_water_stack():
    log("--- WATER STACK ---")
    master = create_or_get_master_material()
    if master is None:
        log("FAIL: could not create or get master material")
        return False
    if not author_master_graph(master):
        return False
    reparent_instance(OCEAN_INSTANCE_PATH, master)
    reparent_instance(LAKE_INSTANCE_PATH, master)
    return True


def fix_caustic_material():
    log("--- CAUSTIC ---")
    mat = unreal.load_asset(CAUSTIC_MATERIAL_PATH)
    if mat is None:
        log("FAIL: caustic missing")
        return False
    if not isinstance(mat, unreal.Material):
        log(f"FAIL: caustic is {type(mat).__name__}")
        return False

    if _has_expressions(mat):
        log("caustic already has expressions; skipping")
        return True

    lib = unreal.MaterialEditingLibrary
    try:
        mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
        mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
        log("set decal domain + translucent blend")
    except Exception as e:
        log(f"WARN setting domain/blend: {e}")

    tint = lib.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, 0)
    tint.set_editor_property("constant", unreal.LinearColor(0.55, 0.7, 0.85, 1.0))
    lib.connect_material_property(tint, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    opacity = lib.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 120)
    opacity.set_editor_property("r", 0.18)
    lib.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)

    lib.recompile_material(mat)
    saved = unreal.EditorAssetLibrary.save_asset(CAUSTIC_MATERIAL_PATH, only_if_is_dirty=False)
    log(f"caustic saved: {saved}")
    return True


def main():
    log("=== START ===")
    try:
        water_ok = fix_water_stack()
    except Exception as e:
        log(f"EXCEPTION in water stack: {e}\n{traceback.format_exc()}")
        water_ok = False
    try:
        caustic_ok = fix_caustic_material()
    except Exception as e:
        log(f"EXCEPTION in caustic: {e}\n{traceback.format_exc()}")
        caustic_ok = False
    log(f"=== DONE water={water_ok} caustic={caustic_ok} ===")


main()

# Quit the editor when running in -unattended mode so the host harness can
# move on. Wrapped because quit_editor only exists in editor builds.
try:
    unreal.SystemLibrary.quit_editor()
except Exception:
    try:
        unreal.EditorLevelLibrary.editor_request_end_play()
    except Exception:
        pass
    import sys
    sys.exit(0)
