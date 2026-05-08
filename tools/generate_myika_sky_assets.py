import sys
import unreal


PLUGIN_ROOT = "/Myika/MyikaSky"
MATERIAL_DIR = f"{PLUGIN_ROOT}/Materials"
DEBUG_DIR = f"{PLUGIN_ROOT}/Debug"
MAP_PATH = f"{PLUGIN_ROOT}/L_SkyTest"
DEBUG_WIDGET_PATH = f"{DEBUG_DIR}/EUW_SkyDebug"
MASTER_MATERIAL_PATH = f"{MATERIAL_DIR}/M_MyikaCloud"
MATERIAL_INSTANCE_PATH = f"{MATERIAL_DIR}/MI_MyikaCloud_Default"

BASE_NOISE_PATH = "/Engine/EngineSky/VolumetricClouds/T_Volume_PerlinWorley_Balanced.T_Volume_PerlinWorley_Balanced"
DETAIL_NOISE_PATH = "/Engine/EngineSky/VolumetricClouds/T_VolumeNoiseErosion32.T_VolumeNoiseErosion32"


def log(message: str) -> None:
    unreal.log(f"[MyikaSkyGen] {message}")


def fail(message: str) -> None:
    unreal.log_error(f"[MyikaSkyGen] {message}")
    raise RuntimeError(message)


def ensure_directory(path: str) -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def asset_exists(path: str) -> bool:
    for candidate in asset_path_candidates(path):
        if unreal.EditorAssetLibrary.does_asset_exist(candidate):
            return True
    return False


def find_existing_asset_path(path: str):
    existing_asset = try_load_asset(path)
    if existing_asset is not None:
        return path

    package_path, asset_name = path.rsplit("/", 1)
    for existing_path in unreal.EditorAssetLibrary.list_assets(package_path, recursive=False, include_folder=False):
        if existing_path.endswith(f"/{asset_name}") or existing_path.endswith(f"/{asset_name}.{asset_name}"):
            return existing_path
    return None


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


def try_load_asset(path: str):
    for candidate in asset_path_candidates(path):
        asset = unreal.load_asset(candidate)
        if asset is None:
            asset = unreal.EditorAssetLibrary.load_asset(candidate)
        if asset is not None:
            return asset
    return None


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


def create_or_load_asset(asset_path: str, asset_class, factory):
    asset_name = asset_path.rsplit("/", 1)[-1]
    package_path = asset_path.rsplit("/", 1)[0]
    existing_asset = try_load_asset(asset_path)
    if existing_asset is not None:
        return existing_asset

    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, package_path, asset_class, factory
    )
    if asset is None:
        fail(f"Failed to create asset: {asset_path}")
    return asset


def create_expression(material, expression_class, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(
        material, expression_class, x, y
    )


def set_prop(obj, name: str, value) -> None:
    last_error = None
    for candidate in property_name_candidates(name):
        try:
            obj.set_editor_property(candidate, value)
            return
        except Exception as exc:
            last_error = exc
    fail(f"Failed setting {name} on {obj.get_name()}: {last_error}")


def build_cloud_material():
    ensure_directory(PLUGIN_ROOT)
    ensure_directory(MATERIAL_DIR)

    material = create_or_load_asset(
        MASTER_MATERIAL_PATH, unreal.Material, unreal.MaterialFactoryNew()
    )
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)

    set_prop(material, "material_domain", unreal.MaterialDomain.MD_VOLUME)
    set_prop(material, "blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    set_prop(material, "used_with_volumetric_cloud", True)
    set_prop(material, "two_sided", False)

    base_noise_texture = load_asset(BASE_NOISE_PATH)
    detail_noise_texture = load_asset(DETAIL_NOISE_PATH)

    world_pos = create_expression(material, unreal.MaterialExpressionWorldPosition, -1800, -300)
    time_node = create_expression(material, unreal.MaterialExpressionTime, -1800, 0)

    base_scale = create_expression(material, unreal.MaterialExpressionConstant, -1800, 220)
    detail_scale = create_expression(material, unreal.MaterialExpressionConstant, -1800, 320)
    wind_pan_scale = create_expression(material, unreal.MaterialExpressionConstant, -1800, 420)
    one_const = create_expression(material, unreal.MaterialExpressionConstant, -1800, 520)
    neg_one_const = create_expression(material, unreal.MaterialExpressionConstant, -1800, 620)
    fluffy_gain = create_expression(material, unreal.MaterialExpressionConstant, -1800, 720)
    density_gain = create_expression(material, unreal.MaterialExpressionConstant, -1800, 820)

    set_prop(base_scale, "r", 0.00015)
    set_prop(detail_scale, "r", 0.0006)
    set_prop(wind_pan_scale, "r", 0.02)
    set_prop(one_const, "r", 1.0)
    set_prop(neg_one_const, "r", -1.0)
    set_prop(fluffy_gain, "r", 1.35)
    set_prop(density_gain, "r", 3.0)

    coverage = create_expression(material, unreal.MaterialExpressionScalarParameter, -1500, -700)
    morphology = create_expression(material, unreal.MaterialExpressionScalarParameter, -1500, -600)
    wind_strength = create_expression(material, unreal.MaterialExpressionScalarParameter, -1500, -500)
    extinction = create_expression(material, unreal.MaterialExpressionScalarParameter, -1500, -400)
    detail = create_expression(material, unreal.MaterialExpressionScalarParameter, -1500, -300)
    wind_direction = create_expression(material, unreal.MaterialExpressionVectorParameter, -1500, -150)
    base_color = create_expression(material, unreal.MaterialExpressionVectorParameter, -1500, 20)

    set_prop(coverage, "parameter_name", "Coverage")
    set_prop(coverage, "default_value", 0.4)
    set_prop(morphology, "parameter_name", "Morphology")
    set_prop(morphology, "default_value", 0.5)
    set_prop(wind_strength, "parameter_name", "WindStrength")
    set_prop(wind_strength, "default_value", 0.1)
    set_prop(extinction, "parameter_name", "Extinction")
    set_prop(extinction, "default_value", 0.5)
    set_prop(detail, "parameter_name", "Detail")
    set_prop(detail, "default_value", 0.5)
    set_prop(wind_direction, "parameter_name", "WindDirection")
    set_prop(wind_direction, "default_value", unreal.LinearColor(1.0, 0.0, 0.0, 1.0))
    set_prop(base_color, "parameter_name", "BaseColor")
    set_prop(base_color, "default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))

    wind_vec = create_expression(material, unreal.MaterialExpressionMultiply, -1200, -180)
    wind_time = create_expression(material, unreal.MaterialExpressionMultiply, -960, -180)
    wind_offset = create_expression(material, unreal.MaterialExpressionMultiply, -720, -180)

    base_coords_scaled = create_expression(material, unreal.MaterialExpressionMultiply, -1200, 240)
    detail_coords_scaled = create_expression(material, unreal.MaterialExpressionMultiply, -1200, 380)
    base_coords = create_expression(material, unreal.MaterialExpressionAdd, -960, 240)
    detail_coords = create_expression(material, unreal.MaterialExpressionAdd, -960, 380)

    base_noise = create_expression(material, unreal.MaterialExpressionTextureSampleParameterVolume, -700, 220)
    detail_noise = create_expression(material, unreal.MaterialExpressionTextureSampleParameterVolume, -700, 400)
    set_prop(base_noise, "parameter_name", "BaseNoiseVolume")
    set_prop(detail_noise, "parameter_name", "DetailNoiseVolume")
    set_prop(base_noise, "texture", base_noise_texture)
    set_prop(detail_noise, "texture", detail_noise_texture)

    detail_blend = create_expression(material, unreal.MaterialExpressionLinearInterpolate, -420, 360)
    shaped_base = create_expression(material, unreal.MaterialExpressionMultiply, -180, 250)
    morph_wispy = create_expression(material, unreal.MaterialExpressionMultiply, 60, 250)
    morph_fluffy = create_expression(material, unreal.MaterialExpressionMultiply, 60, 380)
    morph_shape = create_expression(material, unreal.MaterialExpressionLinearInterpolate, 320, 310)
    coverage_sum = create_expression(material, unreal.MaterialExpressionAdd, 580, 310)
    coverage_bias = create_expression(material, unreal.MaterialExpressionAdd, 820, 310)
    # Patched 2026-05-05: original was MaterialExpressionClamp with min_default=0.0,
    # max_default=1.0 properties and an "Input" pin connection. UE5.7 reports
    # `(Node Clamp) Missing Clamp input` when this is loaded — the Clamp node's
    # Input pin name + min_default/max_default fallback does not bind correctly
    # in the editor's SM6 graph translator. Saturate is exactly clamp(x, 0, 1)
    # with a single Input pin and no Min/Max ambiguity, which is what this node
    # was configured to do anyway. Replacing with Saturate eliminates the
    # compile error without changing semantics.
    coverage_clamp = create_expression(material, unreal.MaterialExpressionSaturate, 1060, 310)
    ext_scale = create_expression(material, unreal.MaterialExpressionMultiply, 580, 520)
    extinction_value = create_expression(material, unreal.MaterialExpressionMultiply, 1320, 420)
    advanced_output = create_expression(
        material, unreal.MaterialExpressionVolumetricAdvancedMaterialOutput, 1320, 120
    )
    set_prop(advanced_output, "b_ground_contribution", True)

    unreal.MaterialEditingLibrary.connect_material_expressions(wind_direction, "", wind_vec, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(wind_strength, "", wind_vec, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(wind_vec, "", wind_time, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(time_node, "", wind_time, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(wind_time, "", wind_offset, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(wind_pan_scale, "", wind_offset, "B")

    unreal.MaterialEditingLibrary.connect_material_expressions(world_pos, "", base_coords_scaled, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(base_scale, "", base_coords_scaled, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(world_pos, "", detail_coords_scaled, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(detail_scale, "", detail_coords_scaled, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(base_coords_scaled, "", base_coords, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(wind_offset, "", base_coords, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(detail_coords_scaled, "", detail_coords, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(wind_offset, "", detail_coords, "B")

    unreal.MaterialEditingLibrary.connect_material_expressions(base_coords, "", base_noise, "Coordinates")
    unreal.MaterialEditingLibrary.connect_material_expressions(detail_coords, "", detail_noise, "Coordinates")

    unreal.MaterialEditingLibrary.connect_material_expressions(one_const, "", detail_blend, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(detail_noise, "R", detail_blend, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(detail, "", detail_blend, "Alpha")

    unreal.MaterialEditingLibrary.connect_material_expressions(base_noise, "R", shaped_base, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(detail_blend, "", shaped_base, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(shaped_base, "", morph_wispy, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(detail_noise, "R", morph_wispy, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(base_noise, "R", morph_fluffy, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(fluffy_gain, "", morph_fluffy, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(morph_wispy, "", morph_shape, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(morph_fluffy, "", morph_shape, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(morphology, "", morph_shape, "Alpha")

    unreal.MaterialEditingLibrary.connect_material_expressions(morph_shape, "", coverage_sum, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(coverage, "", coverage_sum, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(coverage_sum, "", coverage_bias, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(neg_one_const, "", coverage_bias, "B")
    # Patched 2026-05-05: was `connect_material_expressions(... coverage_clamp, "Input")`
    # for MaterialExpressionClamp. Saturate's input pin is unnamed (default), so use "".
    unreal.MaterialEditingLibrary.connect_material_expressions(coverage_bias, "", coverage_clamp, "")

    unreal.MaterialEditingLibrary.connect_material_expressions(extinction, "", ext_scale, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(density_gain, "", ext_scale, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(coverage_clamp, "", extinction_value, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(ext_scale, "", extinction_value, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(coverage_clamp, "", advanced_output, "ConservativeDensity")

    unreal.MaterialEditingLibrary.connect_material_property(
        base_color, "", unreal.MaterialProperty.MP_BASE_COLOR
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        extinction_value, "", unreal.MaterialProperty.MP_OPACITY
    )

    unreal.MaterialEditingLibrary.layout_material_expressions(material)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    log(f"Built material {MASTER_MATERIAL_PATH}")
    return material


def build_material_instance(parent_material):
    ensure_directory(MATERIAL_DIR)

    instance = create_or_load_asset(
        MATERIAL_INSTANCE_PATH,
        unreal.MaterialInstanceConstant,
        unreal.MaterialInstanceConstantFactoryNew(),
    )

    unreal.MaterialEditingLibrary.set_material_instance_parent(instance, parent_material)
    unreal.MaterialEditingLibrary.clear_all_material_instance_parameters(instance)
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(instance, "Coverage", 0.4)
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(instance, "Morphology", 0.5)
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(instance, "WindStrength", 0.1)
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(instance, "Extinction", 0.5)
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(instance, "Detail", 0.5)
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
        instance, "WindDirection", unreal.LinearColor(1.0, 0.0, 0.0, 1.0)
    )
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
        instance, "BaseColor", unreal.LinearColor(1.0, 1.0, 1.0, 1.0)
    )
    unreal.MaterialEditingLibrary.update_material_instance(instance)
    unreal.EditorAssetLibrary.save_loaded_asset(instance)
    log(f"Built material instance {MATERIAL_INSTANCE_PATH}")
    return instance


def build_debug_widget():
    ensure_directory(DEBUG_DIR)

    existing_widget_path = find_existing_asset_path(DEBUG_WIDGET_PATH)
    if existing_widget_path is not None:
        widget = try_load_asset(existing_widget_path)
        if widget is not None:
            log(f"Loaded debug widget {existing_widget_path}")
            return widget

        log(f"Debug widget already exists; skipping recreation for {existing_widget_path}")
        return None

    widget_parent_class = unreal.load_class(None, "/Script/MyikaSkyEditor.MyikaSkyDebugWidget")
    if widget_parent_class is None:
        log("Skipping debug widget creation because /Script/MyikaSkyEditor.MyikaSkyDebugWidget did not load.")
        return None

    factory = unreal.EditorUtilityWidgetBlueprintFactory()
    set_prop(factory, "parent_class", widget_parent_class)

    widget = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "EUW_SkyDebug",
        DEBUG_DIR,
        unreal.EditorUtilityWidgetBlueprint,
        factory,
    )
    if widget is None:
        log(f"Skipping debug widget creation because asset creation failed: {DEBUG_WIDGET_PATH}")
        return None

    unreal.EditorAssetLibrary.save_loaded_asset(widget)
    log(f"Built debug widget {DEBUG_WIDGET_PATH}")
    return widget


def build_map(material_instance):
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    if world is None:
        fail("Failed to create a blank editor map.")

    editor_actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if editor_actor_subsystem is None:
        fail("EditorActorSubsystem is unavailable.")

    sky_actor_class = unreal.load_class(None, "/Script/MyikaSky.MyikaSkyActor")
    if sky_actor_class is None:
        fail("Failed to load /Script/MyikaSky.MyikaSkyActor")

    sky_actor = editor_actor_subsystem.spawn_actor_from_class(
        sky_actor_class, unreal.Vector(0.0, 0.0, 0.0), unreal.Rotator(0.0, 0.0, 0.0)
    )
    if sky_actor is None:
        fail("Failed to spawn AMyikaSkyActor into L_SkyTest.")

    sky_actor.set_actor_label("BP_MyikaSkyActor")
    cloud_component = sky_actor.get_editor_property("volumetric_cloud_component")
    if cloud_component is not None:
        cloud_component.set_material(material_instance)

    plane_mesh = load_asset("/Engine/BasicShapes/Plane")
    floor_actor = editor_actor_subsystem.spawn_actor_from_class(
        unreal.StaticMeshActor, unreal.Vector(0.0, 0.0, 0.0), unreal.Rotator(0.0, 0.0, 0.0)
    )
    floor_actor.set_actor_label("SkyTestFloor")
    floor_component = floor_actor.get_component_by_class(unreal.StaticMeshComponent)
    floor_component.set_static_mesh(plane_mesh)
    floor_actor.set_actor_scale3d(unreal.Vector(100.0, 100.0, 1.0))

    player_start = editor_actor_subsystem.spawn_actor_from_class(
        unreal.PlayerStart, unreal.Vector(0.0, 0.0, 150.0), unreal.Rotator(0.0, 0.0, 0.0)
    )
    player_start.set_actor_label("SkyTestPlayerStart")

    if not unreal.EditorLoadingAndSavingUtils.save_map(world, MAP_PATH):
        fail(f"Failed to save map to {MAP_PATH}")

    log(f"Built map {MAP_PATH}")


def main():
    log("Starting Myika sky asset generation.")
    material = build_cloud_material()
    material_instance = build_material_instance(material)
    build_debug_widget()
    build_map(material_instance)
    unreal.EditorAssetLibrary.save_directory(PLUGIN_ROOT, only_if_is_dirty=False, recursive=True)
    log("Myika sky asset generation completed successfully.")


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        fail(str(exc))
        sys.exit(1)
