import json
import os
import traceback

import unreal


PLUGIN_ROOT = "/Myika/MyikaWater"
MATERIAL_DIR = f"{PLUGIN_ROOT}/Materials"
NIAGARA_DIR = f"{PLUGIN_ROOT}/Niagara"
DEBUG_DIR = f"{PLUGIN_ROOT}/Debug"
MAP_PATH = f"{PLUGIN_ROOT}/L_WaterTest"
MASTER_MATERIAL_PATH = f"{MATERIAL_DIR}/M_MyikaWater"
OCEAN_INSTANCE_PATH = f"{MATERIAL_DIR}/MI_MyikaWater_Ocean"
LAKE_INSTANCE_PATH = f"{MATERIAL_DIR}/MI_MyikaWater_Lake"
RIPPLE_RT_PATH = f"{MATERIAL_DIR}/RT_MyikaShorelineRipple"
RIPPLE_SYSTEM_PATH = f"{NIAGARA_DIR}/NS_MyikaShorelineRipple"
TEST_SWIMMER_PATH = f"{DEBUG_DIR}/BP_TestSwimmer"

OCEAN_MATERIAL_CANDIDATES = [
    "/Water/Materials/WaterSurface/Water_Material_Ocean.Water_Material_Ocean",
    "/Water/Materials/WaterSurface/Water_Material.Water_Material",
]
LAKE_MATERIAL_CANDIDATES = [
    "/Water/Materials/WaterSurface/Water_Material_Lake.Water_Material_Lake",
    "/Water/Materials/WaterSurface/Water_Material.Water_Material",
]
RIPPLE_SYSTEM_CANDIDATES = [
    "/NiagaraFluids/Templates/Liquid/2D/Systems/ShallowWater/Grid2D_SW_Pool.Grid2D_SW_Pool",
    "/NiagaraFluids/Templates/Liquid/2D/Systems/ShallowWater/Grid2D_SW_Drop.Grid2D_SW_Drop",
    "/WaterAdvanced/Niagara/Systems/Grid2D_SW_WaterBody.Grid2D_SW_WaterBody",
]
THIRD_PERSON_CHARACTER_CANDIDATES = [
    "/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.BP_ThirdPersonCharacter",
    "/Game/ThirdPersonBP/Blueprints/ThirdPersonCharacter.ThirdPersonCharacter",
]

OUTPUT_DIR = os.path.join(unreal.Paths.project_saved_dir(), "Validation", "MYI-70-water-scaffold")
SUMMARY_PATH = os.path.join(OUTPUT_DIR, "summary.json")


def log(message: str) -> None:
    unreal.log(f"[MyikaWaterGen] {message}")


def fail(message: str) -> None:
    unreal.log_error(f"[MyikaWaterGen] {message}")
    raise RuntimeError(message)


def ensure_directory(path: str) -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        if not unreal.EditorAssetLibrary.make_directory(path):
            fail(f"Failed to create content directory: {path}")


def ensure_output_dir() -> None:
    os.makedirs(OUTPUT_DIR, exist_ok=True)


def find_first_existing_asset(candidates):
    for candidate in candidates:
        if unreal.EditorAssetLibrary.does_asset_exist(candidate):
            asset = unreal.EditorAssetLibrary.load_asset(candidate)
            if asset is not None:
                return candidate, asset
    return None, None


def duplicate_asset_if_missing(source_asset_path: str, target_asset_path: str):
    if unreal.EditorAssetLibrary.does_asset_exist(target_asset_path):
        asset = unreal.EditorAssetLibrary.load_asset(target_asset_path)
        if asset is None:
            fail(f"Target asset exists but failed to load: {target_asset_path}")
        return asset

    if not unreal.EditorAssetLibrary.duplicate_asset(source_asset_path, target_asset_path):
        fail(f"Failed to duplicate {source_asset_path} -> {target_asset_path}")

    asset = unreal.EditorAssetLibrary.load_asset(target_asset_path)
    if asset is None:
        fail(f"Duplicated asset failed to load: {target_asset_path}")
    return asset


def create_or_load_material_instance(instance_path: str, parent_material):
    asset_name = instance_path.rsplit("/", 1)[-1]
    package_path = instance_path.rsplit("/", 1)[0]

    if unreal.EditorAssetLibrary.does_asset_exist(instance_path):
        instance = unreal.EditorAssetLibrary.load_asset(instance_path)
        if instance is None:
            fail(f"Asset exists but failed to load: {instance_path}")
    else:
        instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name,
            package_path,
            unreal.MaterialInstanceConstant,
            unreal.MaterialInstanceConstantFactoryNew(),
        )
        if instance is None:
            fail(f"Failed to create material instance: {instance_path}")

    unreal.MaterialEditingLibrary.set_material_instance_parent(instance, parent_material)
    unreal.MaterialEditingLibrary.update_material_instance(instance)
    unreal.EditorAssetLibrary.save_loaded_asset(instance, False)
    return instance


def create_or_load_render_target(rt_path: str):
    asset_name = rt_path.rsplit("/", 1)[-1]
    package_path = rt_path.rsplit("/", 1)[0]

    if unreal.EditorAssetLibrary.does_asset_exist(rt_path):
        render_target = unreal.EditorAssetLibrary.load_asset(rt_path)
        if render_target is None:
            fail(f"Asset exists but failed to load: {rt_path}")
    else:
        render_target = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name,
            package_path,
            unreal.TextureRenderTarget2D,
            unreal.TextureRenderTargetFactoryNew(),
        )
        if render_target is None:
            fail(f"Failed to create render target: {rt_path}")

    if hasattr(render_target, "resize_target"):
        render_target.resize_target(1024, 1024)

    if hasattr(render_target, "set_editor_property"):
        try:
            render_target.set_editor_property("clear_color", unreal.LinearColor(0.5, 0.5, 1.0, 1.0))
        except Exception:
            pass

    unreal.EditorAssetLibrary.save_loaded_asset(render_target, False)
    return render_target


def duplicate_test_swimmer():
    source_path, _asset = find_first_existing_asset(THIRD_PERSON_CHARACTER_CANDIDATES)
    if source_path is None:
        return None
    return duplicate_asset_if_missing(source_path, TEST_SWIMMER_PATH)


def build_map():
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    if world is None:
        fail("Failed to create blank map for L_WaterTest")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if actor_subsystem is None:
        fail("EditorActorSubsystem is unavailable")

    water_controller_class = unreal.load_class(None, "/Script/MyikaWater.MyikaWaterController")
    ocean_class = unreal.load_class(None, "/Script/MyikaWater.MyikaOcean")
    if water_controller_class is None or ocean_class is None:
        fail("Failed to load MyikaWater runtime classes for map generation")

    controller = actor_subsystem.spawn_actor_from_class(
        water_controller_class,
        unreal.Vector(0.0, 0.0, 0.0),
        unreal.Rotator(0.0, 0.0, 0.0),
    )
    if controller is not None:
        controller.set_actor_label("BP_MyikaWaterController")

    ocean = actor_subsystem.spawn_actor_from_class(
        ocean_class,
        unreal.Vector(0.0, 0.0, 0.0),
        unreal.Rotator(0.0, 0.0, 0.0),
    )
    if ocean is not None:
        ocean.set_actor_label("BP_MyikaOcean")

    plane_mesh = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Plane.Plane")
    if plane_mesh is not None:
        seabed = actor_subsystem.spawn_actor_from_class(
            unreal.StaticMeshActor,
            unreal.Vector(0.0, 0.0, -200.0),
            unreal.Rotator(0.0, 0.0, 0.0),
        )
        seabed.set_actor_label("WaterTestSeabed")
        mesh_component = seabed.get_component_by_class(unreal.StaticMeshComponent)
        if mesh_component is not None:
            mesh_component.set_static_mesh(plane_mesh)
        seabed.set_actor_scale3d(unreal.Vector(120.0, 120.0, 1.0))

    player_start = actor_subsystem.spawn_actor_from_class(
        unreal.PlayerStart,
        unreal.Vector(0.0, 0.0, 220.0),
        unreal.Rotator(0.0, 90.0, 0.0),
    )
    if player_start is not None:
        player_start.set_actor_label("WaterTestPlayerStart")

    if not unreal.EditorLoadingAndSavingUtils.save_map(world, MAP_PATH):
        fail(f"Failed to save map: {MAP_PATH}")

    return MAP_PATH


def save_directory(path: str) -> None:
    unreal.EditorAssetLibrary.save_directory(path, only_if_is_dirty=False, recursive=True)


def write_summary(data) -> None:
    with open(SUMMARY_PATH, "w", encoding="utf-8") as handle:
        json.dump(data, handle, indent=2)


def main():
    ensure_output_dir()
    ensure_directory(PLUGIN_ROOT)
    ensure_directory(MATERIAL_DIR)
    ensure_directory(NIAGARA_DIR)
    ensure_directory(DEBUG_DIR)

    ocean_source_path, ocean_source = find_first_existing_asset(OCEAN_MATERIAL_CANDIDATES)
    lake_source_path, lake_source = find_first_existing_asset(LAKE_MATERIAL_CANDIDATES)
    ripple_source_path, ripple_source = find_first_existing_asset(RIPPLE_SYSTEM_CANDIDATES)

    if ocean_source is None:
        fail("Could not locate an engine water material to duplicate")
    if ripple_source is None:
        fail("Could not locate a Niagara shallow-water template to duplicate")

    master_material = duplicate_asset_if_missing(ocean_source_path, MASTER_MATERIAL_PATH)
    ocean_instance = create_or_load_material_instance(OCEAN_INSTANCE_PATH, master_material)
    lake_parent = lake_source if lake_source is not None else master_material
    lake_instance = create_or_load_material_instance(LAKE_INSTANCE_PATH, lake_parent)
    render_target = create_or_load_render_target(RIPPLE_RT_PATH)
    ripple_system = duplicate_asset_if_missing(ripple_source_path, RIPPLE_SYSTEM_PATH)
    swimmer_blueprint = duplicate_test_swimmer()
    map_path = build_map()

    save_directory(PLUGIN_ROOT)

    write_summary(
        {
            "status": "ok",
            "master_material": master_material.get_path_name(),
            "ocean_instance": ocean_instance.get_path_name(),
            "lake_instance": lake_instance.get_path_name(),
            "ripple_render_target": render_target.get_path_name(),
            "ripple_system": ripple_system.get_path_name(),
            "test_swimmer": swimmer_blueprint.get_path_name() if swimmer_blueprint is not None else None,
            "map": map_path,
            "sources": {
                "ocean_material": ocean_source_path,
                "lake_material": lake_source_path,
                "ripple_system": ripple_source_path,
            },
            "notes": [
                "Generated UE-native scaffold assets only.",
                "Ripple material sampling and BP_TestSwimmer component wiring still require follow-up editor authoring.",
            ],
        }
    )

    log("MyikaWater scaffold asset generation completed successfully.")


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        ensure_output_dir()
        write_summary(
            {
                "status": "error",
                "message": str(exc),
                "traceback": traceback.format_exc(),
            }
        )
        raise
