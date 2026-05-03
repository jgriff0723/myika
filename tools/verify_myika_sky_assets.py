import sys
import unreal


MASTER_MATERIAL_PATH = "/Myika/MyikaSky/Materials/M_MyikaCloud"
MATERIAL_INSTANCE_PATH = "/Myika/MyikaSky/Materials/MI_MyikaCloud_Default"
MAP_PATH = "/Myika/MyikaSky/L_SkyTest"


def log(message: str) -> None:
    unreal.log(f"[MyikaSkyVerify] {message}")


def fail(message: str) -> None:
    unreal.log_error(f"[MyikaSkyVerify] {message}")
    raise RuntimeError(message)


def asset_path_candidates(path: str):
    candidates = [path]
    leaf_name = path.rsplit("/", 1)[-1]
    if "." not in leaf_name:
        candidates.append(f"{path}.{leaf_name}")
    return candidates


def load_asset(path: str):
    for candidate in asset_path_candidates(path):
        asset = unreal.load_asset(candidate)
        if asset is None:
            asset = unreal.EditorAssetLibrary.load_asset(candidate)
        if asset is not None:
            return asset
    fail(f"Unable to load asset: {path}")


def property_name_candidates(name: str):
    candidates = [name]

    if "_" in name:
        parts = [part for part in name.split("_") if part]
        camel = parts[0] + "".join(part.capitalize() for part in parts[1:])
        pascal = "".join(part.capitalize() for part in parts)
        candidates.extend([camel, pascal])
        if parts[0] == "b" and len(parts) > 1:
            candidates.append("b" + "".join(part.capitalize() for part in parts[1:]))

    if name.startswith("b") and len(name) > 1 and name[1].isupper():
        snake = []
        for index, character in enumerate(name):
            if character.isupper() and index > 0:
                snake.append("_")
            snake.append(character.lower())
        candidates.append("".join(snake))

    unique = []
    seen = set()
    for candidate in candidates:
        if candidate and candidate not in seen:
            seen.add(candidate)
            unique.append(candidate)
    return unique


def get_prop(obj, name: str):
    last_error = None
    for candidate in property_name_candidates(name):
        try:
            return obj.get_editor_property(candidate)
        except Exception as exc:
            last_error = exc
    fail(f"Failed reading {name} on {obj.get_name()}: {last_error}")


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def main():
    material = load_asset(MASTER_MATERIAL_PATH)
    instance = load_asset(MATERIAL_INSTANCE_PATH)

    domain = get_prop(material, "material_domain")
    require(domain == unreal.MaterialDomain.MD_VOLUME, "M_MyikaCloud is not a Volume material.")

    used_with_cloud = get_prop(material, "used_with_volumetric_cloud")
    require(bool(used_with_cloud), "M_MyikaCloud is not flagged for volumetric clouds.")

    parent_material = get_prop(instance, "parent")
    require(parent_material is not None, "MI_MyikaCloud_Default has no parent material.")
    require(
        parent_material.get_path_name().endswith("M_MyikaCloud"),
        f"MI_MyikaCloud_Default parent mismatch: {parent_material.get_path_name()}",
    )

    world = unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
    require(world is not None, "Failed to load L_SkyTest.")

    editor_actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    require(editor_actor_subsystem is not None, "EditorActorSubsystem is unavailable.")

    sky_actor = None
    for actor in editor_actor_subsystem.get_all_level_actors():
        if actor.get_class().get_name() == "MyikaSkyActor":
            sky_actor = actor
            break

    require(sky_actor is not None, "L_SkyTest does not contain AMyikaSkyActor.")

    cloud_component = get_prop(sky_actor, "volumetric_cloud_component")
    require(cloud_component is not None, "AMyikaSkyActor has no volumetric cloud component.")

    assigned_material = get_prop(cloud_component, "material")
    require(assigned_material is not None, "Volumetric cloud component has no assigned material.")
    require(
        "MI_MyikaCloud_Default" in assigned_material.get_path_name(),
        f"Volumetric cloud component material mismatch: {assigned_material.get_path_name()}",
    )

    log(
        "VERIFIED "
        f"material={material.get_path_name()} "
        f"instance={instance.get_path_name()} "
        f"map={MAP_PATH} "
        f"actor={sky_actor.get_name()} "
        f"component_material={assigned_material.get_path_name()}"
    )


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        fail(str(exc))
        sys.exit(1)
