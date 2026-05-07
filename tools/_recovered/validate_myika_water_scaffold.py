import glob
import json
import os
import time
import traceback

import unreal


MAP_PATH = "/Myika/MyikaWater/L_WaterTest"
PLUGIN_DIR = "/Myika/MyikaWater"
WATER_CONTROLLER_CLASS_PATH = "/Script/MyikaWater.MyikaWaterController"
MYIKA_SKY_CLASS_PATH = "/Script/MyikaSky.MyikaSkyActor"
WATER_ZONE_CLASS_PATH = "/Script/Water.WaterZone"
WATER_BODY_OCEAN_CLASS_PATH = "/Script/Water.WaterBodyOcean"

VALIDATION_DIR = os.path.join(unreal.Paths.project_saved_dir(), "Validation", "MYI-66")
SUMMARY_PATH = os.path.join(VALIDATION_DIR, "water_scaffold_validation.json")
SCREENSHOT_DIR = os.path.join(unreal.Paths.project_saved_dir(), "Screenshots", "WindowsEditor")


def log(message: str) -> None:
    unreal.log(f"[MYI-66] {message}")


def fail(message: str) -> None:
    unreal.log_error(f"[MYI-66] {message}")
    raise RuntimeError(message)


def ensure_directory(path: str) -> None:
    if unreal.EditorAssetLibrary.does_directory_exist(path):
        return
    if not unreal.EditorAssetLibrary.make_directory(path):
        fail(f"Failed to create directory: {path}")


def load_required_class(class_path: str):
    loaded_class = unreal.load_class(None, class_path)
    if loaded_class is None:
        fail(f"Failed to load class: {class_path}")
    return loaded_class


def set_static_mesh(actor, mesh_path: str) -> None:
    mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
    if mesh is None:
        fail(f"Failed to load static mesh: {mesh_path}")

    static_mesh_component = actor.get_component_by_class(unreal.StaticMeshComponent)
    if static_mesh_component is None:
        fail(f"Actor {actor.get_name()} is missing a StaticMeshComponent")

    static_mesh_component.set_static_mesh(mesh)


def create_map_if_missing(result: dict) -> None:
    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        result["map_created"] = False
        return

    ensure_directory(PLUGIN_DIR)

    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    if world is None:
        fail("Failed to create blank map for L_WaterTest")

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if actor_subsystem is None:
        fail("EditorActorSubsystem is unavailable")

    water_controller_class = load_required_class(WATER_CONTROLLER_CLASS_PATH)
    water_zone_class = load_required_class(WATER_ZONE_CLASS_PATH)
    water_body_ocean_class = load_required_class(WATER_BODY_OCEAN_CLASS_PATH)
    myika_sky_class = unreal.load_class(None, MYIKA_SKY_CLASS_PATH)

    if myika_sky_class is not None:
        sky_actor = actor_subsystem.spawn_actor_from_class(
            myika_sky_class,
            unreal.Vector(0.0, 0.0, 0.0),
            unreal.Rotator(0.0, 0.0, 0.0),
        )
        if sky_actor is not None:
            sky_actor.set_actor_label("WaterTestSky")

    controller = actor_subsystem.spawn_actor_from_class(
        water_controller_class,
        unreal.Vector(0.0, 0.0, 0.0),
        unreal.Rotator(0.0, 0.0, 0.0),
    )
    if controller is None:
        fail("Failed to spawn AMyikaWaterController")
    controller.set_actor_label("WaterTestController")

    water_zone = actor_subsystem.spawn_actor_from_class(
        water_zone_class,
        unreal.Vector(0.0, 0.0, 0.0),
        unreal.Rotator(0.0, 0.0, 0.0),
    )
    if water_zone is None:
        fail("Failed to spawn AWaterZone")
    water_zone.set_actor_label("WaterTestZone")
    try:
        water_zone.set_editor_property("zone_extent", unreal.Vector2D(64000.0, 64000.0))
    except Exception as exc:
        log(f"WaterZone extent update skipped: {exc}")

    ocean = actor_subsystem.spawn_actor_from_class(
        water_body_ocean_class,
        unreal.Vector(0.0, 0.0, 0.0),
        unreal.Rotator(0.0, 0.0, 0.0),
    )
    if ocean is None:
        fail("Failed to spawn AWaterBodyOcean")
    ocean.set_actor_label("WaterTestOcean")

    try:
        water_body_component = ocean.get_component_by_class(unreal.WaterBodyOceanComponent)
        if water_body_component is not None:
            try:
                water_body_component.set_editor_property(
                    "collision_extents",
                    unreal.Vector(64000.0, 64000.0, 2048.0),
                )
            except Exception as exc:
                log(f"Ocean collision extents update skipped: {exc}")
    except Exception as exc:
        log(f"Ocean component update skipped: {exc}")

    seabed = actor_subsystem.spawn_actor_from_class(
        unreal.StaticMeshActor,
        unreal.Vector(0.0, 0.0, -650.0),
        unreal.Rotator(0.0, 0.0, 0.0),
    )
    if seabed is None:
        fail("Failed to spawn seabed mesh actor")
    seabed.set_actor_label("WaterTestSeabed")
    set_static_mesh(seabed, "/Engine/BasicShapes/Cube.Cube")
    seabed.set_actor_scale3d(unreal.Vector(320.0, 320.0, 12.0))

    shore_ramp = actor_subsystem.spawn_actor_from_class(
        unreal.StaticMeshActor,
        unreal.Vector(-2600.0, 0.0, -40.0),
        unreal.Rotator(-4.0, 0.0, 0.0),
    )
    if shore_ramp is None:
        fail("Failed to spawn shoreline ramp actor")
    shore_ramp.set_actor_label("WaterTestShoreRamp")
    set_static_mesh(shore_ramp, "/Engine/BasicShapes/Cube.Cube")
    shore_ramp.set_actor_scale3d(unreal.Vector(80.0, 26.0, 2.0))

    player_start = actor_subsystem.spawn_actor_from_class(
        unreal.PlayerStart,
        unreal.Vector(-5200.0, 0.0, 260.0),
        unreal.Rotator(0.0, 0.0, 0.0),
    )
    if player_start is None:
        fail("Failed to spawn PlayerStart")
    player_start.set_actor_label("WaterTestPlayerStart")

    if not unreal.EditorLoadingAndSavingUtils.save_map(world, MAP_PATH):
        fail(f"Failed to save map: {MAP_PATH}")

    unreal.EditorAssetLibrary.save_directory(PLUGIN_DIR, only_if_is_dirty=False, recursive=True)
    result["map_created"] = True


def main() -> None:
    os.makedirs(VALIDATION_DIR, exist_ok=True)
    os.makedirs(SCREENSHOT_DIR, exist_ok=True)

    result = {
        "map_path": MAP_PATH,
        "summary_path": SUMMARY_PATH.replace("\\", "/"),
        "validation_dir": VALIDATION_DIR.replace("\\", "/"),
        "screenshot_dir": SCREENSHOT_DIR.replace("\\", "/"),
    }

    try:
        create_map_if_missing(result)

        level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        if level_subsystem is None:
            fail("LevelEditorSubsystem is unavailable")

        if not level_subsystem.load_level(MAP_PATH):
            fail(f"Failed to load level: {MAP_PATH}")

        water_controller_class = load_required_class(WATER_CONTROLLER_CLASS_PATH)
        water_zone_class = load_required_class(WATER_ZONE_CLASS_PATH)
        water_body_ocean_class = load_required_class(WATER_BODY_OCEAN_CLASS_PATH)

        world = unreal.EditorLevelLibrary.get_editor_world()
        if world is None:
            fail("Editor world is unavailable after loading L_WaterTest")

        water_controllers = unreal.GameplayStatics.get_all_actors_of_class(world, water_controller_class)
        water_zones = unreal.GameplayStatics.get_all_actors_of_class(world, water_zone_class)
        oceans = unreal.GameplayStatics.get_all_actors_of_class(world, water_body_ocean_class)
        player_starts = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.PlayerStart)

        result["editor_actor_counts"] = {
            "water_controllers": len(water_controllers),
            "water_zones": len(water_zones),
            "oceans": len(oceans),
            "player_starts": len(player_starts),
        }

        if not water_controllers:
            fail("L_WaterTest does not contain AMyikaWaterController")
        if not water_zones:
            fail("L_WaterTest does not contain AWaterZone")
        if not oceans:
            fail("L_WaterTest does not contain AWaterBodyOcean")
        if not player_starts:
            fail("L_WaterTest does not contain a PlayerStart")

        level_subsystem.editor_set_game_view(True)
        before_shots = set(glob.glob(os.path.join(SCREENSHOT_DIR, "*.png")))

        level_subsystem.editor_request_begin_play()
        pie_start_deadline = time.time() + 30.0
        while not level_subsystem.is_in_play_in_editor() and time.time() < pie_start_deadline:
            time.sleep(0.5)

        if not level_subsystem.is_in_play_in_editor():
            fail("PIE did not start before timeout")

        pie_worlds = unreal.EditorLevelLibrary.get_pie_worlds(False)
        if not pie_worlds:
            fail("No PIE worlds found after starting PIE")

        pie_world = pie_worlds[0]
        player_controller = unreal.GameplayStatics.get_player_controller(pie_world, 0)
        player_pawn = unreal.GameplayStatics.get_player_pawn(pie_world, 0)
        pie_oceans = unreal.GameplayStatics.get_all_actors_of_class(pie_world, water_body_ocean_class)
        pie_controllers = unreal.GameplayStatics.get_all_actors_of_class(pie_world, water_controller_class)

        result["pie_started"] = True
        result["pie_actor_counts"] = {
            "water_controllers": len(pie_controllers),
            "oceans": len(pie_oceans),
        }
        result["pawn_class"] = player_pawn.get_class().get_name() if player_pawn else None
        result["pawn_location"] = list(player_pawn.get_actor_location()) if player_pawn else None
        result["water_controller_class_loaded"] = water_controller_class.get_name()

        if player_controller:
            unreal.SystemLibrary.execute_console_command(player_controller, "HighResShot 1280x720")

        time.sleep(5.0)
        pie_end_time = time.time() + 25.0
        while time.time() < pie_end_time:
            time.sleep(1.0)

        after_shots = set(glob.glob(os.path.join(SCREENSHOT_DIR, "*.png")))
        new_shots = sorted(after_shots - before_shots, key=os.path.getmtime)
        result["screenshot_path"] = new_shots[-1].replace("\\", "/") if new_shots else None

        level_subsystem.editor_request_end_play()
        time.sleep(1.0)
        result["success"] = True
    except Exception:
        result["success"] = False
        result["error"] = traceback.format_exc()
    finally:
        with open(SUMMARY_PATH, "w", encoding="utf-8") as handle:
            json.dump(result, handle, indent=2)
        unreal.log(json.dumps(result))
        unreal.SystemLibrary.quit_editor()


if __name__ == "__main__":
    main()
