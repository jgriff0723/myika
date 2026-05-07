"""
Reparent the Myika water MICs to UE Water plugin's reference Water_Material
instead of the from-scratch M_MyikaWaterMaster. The custom master rendered
white-green because Single Layer Water needs scattering/absorption/PhaseG
outputs that MaterialEditingLibrary cannot easily wire from Python.

Engine Water_Material at /Water/Materials/WaterSurface/Water_Material is
already wired correctly. We override BaseColor / Roughness etc as MI
scalar/vector parameter values on top.

Run via:
    UnrealEditor-Cmd.exe <project> -nullrhi -unattended
        -ExecCmds="py <this-file>" -log -abslog=<log>
"""

import traceback
import unreal


ENGINE_WATER_MATERIAL_PATH = "/Water/Materials/WaterSurface/Water_Material"
ENGINE_LAKE_MATERIAL_PATH = "/Water/Materials/WaterSurface/Water_Material_Lake"
OCEAN_INSTANCE_PATH = "/Myika/MyikaWater/Materials/MI_MyikaWater_Ocean"
LAKE_INSTANCE_PATH = "/Myika/MyikaWater/Materials/MI_MyikaWater_Lake"


def log(msg):
    unreal.log(f"[reparent_to_engine_water] {msg}")


def reparent(instance_path, parent_path):
    log(f"loading {instance_path}")
    mic = unreal.load_asset(instance_path)
    if mic is None:
        log(f"FAIL: {instance_path} not found")
        return False
    if not isinstance(mic, unreal.MaterialInstanceConstant):
        log(f"FAIL: {instance_path} is {type(mic).__name__}")
        return False

    log(f"loading parent {parent_path}")
    parent = unreal.load_asset(parent_path)
    if parent is None:
        log(f"FAIL: parent {parent_path} not found (UE Water plugin may not have loaded the asset yet)")
        return False

    current = mic.get_editor_property("parent")
    current_name = current.get_path_name() if current else "<none>"
    log(f"current parent: {current_name}")
    if current and current.get_path_name() == parent.get_path_name():
        log("already correct, skipping")
        return True

    mic.set_editor_property("parent", parent)
    saved = unreal.EditorAssetLibrary.save_loaded_asset(mic, only_if_is_dirty=False)
    log(f"reparented + save_loaded_asset: {saved}")
    if not saved:
        saved = unreal.EditorAssetLibrary.save_asset(instance_path, only_if_is_dirty=False)
        log(f"  fallback save_asset: {saved}")
    return saved


def main():
    log("=== START ===")
    try:
        ocean_ok = reparent(OCEAN_INSTANCE_PATH, ENGINE_WATER_MATERIAL_PATH)
    except Exception as e:
        log(f"EXCEPTION ocean: {e}\n{traceback.format_exc()}")
        ocean_ok = False
    try:
        lake_ok = reparent(LAKE_INSTANCE_PATH, ENGINE_LAKE_MATERIAL_PATH)
    except Exception as e:
        log(f"EXCEPTION lake: {e}\n{traceback.format_exc()}")
        lake_ok = False
    log(f"=== DONE ocean={ocean_ok} lake={lake_ok} ===")


main()

try:
    unreal.SystemLibrary.quit_editor()
except Exception:
    import sys
    sys.exit(0)
