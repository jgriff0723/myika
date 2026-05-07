import json
import os
import traceback

import unreal


MAP_PATH = "/Game/ThirdPerson/Lvl_ThirdPerson"
PLANE_MESH_PATH = "/Engine/BasicShapes/Plane.Plane"
MPC_PATH = "/Myika/MyikaCore/MPC_Myika"
DEBUG_MATERIAL_PATH = "/Myika/MyikaCore/Debug/M_MPCDebug"
OUTPUT_DIR = os.path.join(unreal.Paths.project_saved_dir(), "Validation", "MYI-54-mpc")
SUMMARY_PATH = os.path.join(OUTPUT_DIR, "validation.json")
BEFORE_SCREENSHOT_PATH = os.path.join(OUTPUT_DIR, "before.png")
AFTER_SCREENSHOT_PATH = os.path.join(OUTPUT_DIR, "after.png")
DEFAULT_STATE = {
    "TimeOfDay": 12.0,
    "Wetness": 0.0,
}
CHANGED_STATE = {
    "TimeOfDay": 18.0,
    "Wetness": 0.85,
}
ACTOR_LABEL_PREFIX = "MYI54_MPC_DEBUG"
CAMERA_LOCATION = unreal.Vector(-850.0, 0.0, 500.0)
PLANE_LOCATION = unreal.Vector(0.0, 0.0, 200.0)
PLANE_SCALE = unreal.Vector(8.0, 8.0, 8.0)


STATE = {
    "phase": "init",
    "phase_ticks": 0,
    "cooldown_ticks": 0,
    "task": None,
    "callback_handle": None,
    "plane_actor": None,
    "camera_actor": None,
}


def ensure_output_dir():
    os.makedirs(OUTPUT_DIR, exist_ok=True)


def get_world():
    world = unreal.EditorLevelLibrary.get_editor_world()
    if world is None:
        raise RuntimeError("Editor world is unavailable")
    return world


def get_collection():
    collection = unreal.EditorAssetLibrary.load_asset(MPC_PATH)
    if collection is None:
        raise RuntimeError(f"Failed to load collection: {MPC_PATH}")
    return collection


def write_summary(status, extra=None):
    data = {"status": status}
    if extra:
        data.update(extra)

    with open(SUMMARY_PATH, "w", encoding="utf-8") as handle:
        json.dump(data, handle, indent=2)


def finish(status, extra=None):
    if STATE["callback_handle"] is not None:
        unreal.unregister_slate_post_tick_callback(STATE["callback_handle"])
        STATE["callback_handle"] = None

    cleanup_transient_actors()
    write_summary(status, extra)
    world = get_world()
    unreal.SystemLibrary.execute_console_command(world, "QUIT_EDITOR")


def fail(message):
    unreal.log_error(message)
    finish(
        "error",
        {
            "message": message,
            "traceback": traceback.format_exc(),
        },
    )


def cleanup_transient_actors():
    for actor_key in ("plane_actor", "camera_actor"):
        actor = STATE.get(actor_key)
        if actor is not None:
            try:
                unreal.EditorLevelLibrary.destroy_actor(actor)
            except Exception:
                pass
            STATE[actor_key] = None

    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if actor.get_actor_label().startswith(ACTOR_LABEL_PREFIX):
            try:
                unreal.EditorLevelLibrary.destroy_actor(actor)
            except Exception:
                pass


def update_scalar_defaults(collection, replacements):
    parameters = list(collection.get_editor_property("scalar_parameters"))
    seen_names = set()

    for parameter in parameters:
        name = str(parameter.get_editor_property("parameter_name"))
        if name in replacements:
            parameter.set_editor_property("default_value", replacements[name])
            seen_names.add(name)

    missing = sorted(set(replacements.keys()) - seen_names)
    if missing:
        raise RuntimeError(f"Collection is missing scalar parameter(s): {', '.join(missing)}")

    collection.set_editor_property("scalar_parameters", parameters)
    if not unreal.EditorAssetLibrary.save_loaded_asset(collection, False):
        raise RuntimeError(f"Failed to save collection after update: {collection.get_path_name()}")


def setup_scene():
    unreal.EditorLevelLibrary.load_level(MAP_PATH)
    cleanup_transient_actors()

    plane_mesh = unreal.EditorAssetLibrary.load_asset(PLANE_MESH_PATH)
    if plane_mesh is None:
        raise RuntimeError(f"Failed to load mesh: {PLANE_MESH_PATH}")

    debug_material = unreal.EditorAssetLibrary.load_asset(DEBUG_MATERIAL_PATH)
    if debug_material is None:
        raise RuntimeError(f"Failed to load material: {DEBUG_MATERIAL_PATH}")

    plane_actor = unreal.EditorLevelLibrary.spawn_actor_from_object(
        plane_mesh,
        PLANE_LOCATION,
        unreal.Rotator(0.0, 0.0, 0.0),
        True,
    )
    if plane_actor is None:
        raise RuntimeError("Failed to spawn validation plane")

    plane_actor.set_actor_label(f"{ACTOR_LABEL_PREFIX}_Plane")
    plane_actor.set_actor_scale3d(PLANE_SCALE)
    plane_actor.set_actor_enable_collision(False)

    mesh_component = plane_actor.get_component_by_class(unreal.StaticMeshComponent)
    if mesh_component is None:
        raise RuntimeError("Validation plane is missing StaticMeshComponent")

    mesh_component.set_material(0, debug_material)

    camera_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.CameraActor,
        CAMERA_LOCATION,
        unreal.Rotator(0.0, 0.0, 0.0),
        True,
    )
    if camera_actor is None:
        raise RuntimeError("Failed to spawn validation camera")

    camera_actor.set_actor_label(f"{ACTOR_LABEL_PREFIX}_Camera")
    look_at_rotation = unreal.MathLibrary.find_look_at_rotation(CAMERA_LOCATION, PLANE_LOCATION)
    camera_actor.set_actor_rotation(look_at_rotation, False)

    editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    editor.set_level_viewport_camera_info(CAMERA_LOCATION, look_at_rotation)

    STATE["plane_actor"] = plane_actor
    STATE["camera_actor"] = camera_actor


def trigger_screenshot(path):
    task = unreal.AutomationLibrary.take_high_res_screenshot(
        1280,
        720,
        path,
        STATE["camera_actor"],
        False,
        False,
        unreal.ComparisonTolerance.LOW,
        "",
        0.2,
        True,
    )
    if task is None or not task.is_valid_task():
        raise RuntimeError(f"Failed to start screenshot task: {path}")
    return task


def tick(_delta_seconds):
    try:
        STATE["phase_ticks"] += 1

        if STATE["phase"] == "init":
            ensure_output_dir()
            setup_scene()
            update_scalar_defaults(get_collection(), DEFAULT_STATE)
            STATE["task"] = trigger_screenshot(BEFORE_SCREENSHOT_PATH)
            STATE["phase"] = "wait_before"
            STATE["phase_ticks"] = 0
            return

        if STATE["phase"] == "wait_before":
            if STATE["task"].is_task_done():
                update_scalar_defaults(get_collection(), CHANGED_STATE)
                STATE["cooldown_ticks"] = 30
                STATE["phase"] = "settle_after_change"
                STATE["phase_ticks"] = 0
            elif STATE["phase_ticks"] > 900:
                raise RuntimeError("Timed out waiting for before screenshot")
            return

        if STATE["phase"] == "settle_after_change":
            STATE["cooldown_ticks"] -= 1
            if STATE["cooldown_ticks"] <= 0:
                STATE["task"] = trigger_screenshot(AFTER_SCREENSHOT_PATH)
                STATE["phase"] = "wait_after"
                STATE["phase_ticks"] = 0
            return

        if STATE["phase"] == "wait_after":
            if STATE["task"].is_task_done():
                update_scalar_defaults(get_collection(), DEFAULT_STATE)
                finish(
                    "ok",
                    {
                        "map": MAP_PATH,
                        "before": BEFORE_SCREENSHOT_PATH,
                        "after": AFTER_SCREENSHOT_PATH,
                        "baseline": DEFAULT_STATE,
                        "changed": CHANGED_STATE,
                    },
                )
            elif STATE["phase_ticks"] > 900:
                raise RuntimeError("Timed out waiting for after screenshot")
            return

        raise RuntimeError(f"Unexpected validation phase: {STATE['phase']}")
    except Exception as exc:
        fail(str(exc))


def main():
    ensure_output_dir()
    STATE["callback_handle"] = unreal.register_slate_post_tick_callback(tick)
    unreal.log("Registered MYI-54 MPC validation tick callback")


if __name__ == "__main__":
    main()
