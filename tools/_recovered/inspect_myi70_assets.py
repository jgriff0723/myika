import json
import os

import unreal


PLUGIN_ROOT = "/Myika/MyikaWater"
MATERIAL_PATH = f"{PLUGIN_ROOT}/Materials/M_MyikaWater"
OCEAN_INSTANCE_PATH = f"{PLUGIN_ROOT}/Materials/MI_MyikaWater_Ocean"
TEST_SWIMMER_PATH = f"{PLUGIN_ROOT}/Debug/BP_TestSwimmer"
OUTPUT_DIR = os.path.join(unreal.Paths.project_saved_dir(), "Validation", "MYI-70-water-scaffold")
OUTPUT_PATH = os.path.join(OUTPUT_DIR, "inspection.json")


def ensure_output_dir() -> None:
    os.makedirs(OUTPUT_DIR, exist_ok=True)


def load_asset(path: str):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if asset is None:
        raise RuntimeError(f"Failed to load asset: {path}")
    return asset


def name_list(values):
    return [str(value) for value in values]


def inspect_material(path: str):
    material = load_asset(path)
    data = {
        "path": material.get_path_name(),
        "class": material.get_class().get_name(),
        "scalar_parameters": name_list(unreal.MaterialEditingLibrary.get_scalar_parameter_names(material)),
        "vector_parameters": name_list(unreal.MaterialEditingLibrary.get_vector_parameter_names(material)),
        "texture_parameters": name_list(unreal.MaterialEditingLibrary.get_texture_parameter_names(material)),
    }

    if isinstance(material, unreal.Material):
        normal_input = unreal.MaterialEditingLibrary.get_material_property_input_node(
            material,
            unreal.MaterialProperty.MP_NORMAL,
        )
        data["normal_input_node"] = normal_input.get_name() if normal_input else None
        data["normal_input_output"] = (
            unreal.MaterialEditingLibrary.get_material_property_input_node_output_name(
                material,
                unreal.MaterialProperty.MP_NORMAL,
            )
            if normal_input
            else None
        )
        data["expression_count"] = unreal.MaterialEditingLibrary.get_num_material_expressions(material)

    return data


def inspect_swimmer_blueprint():
    blueprint = load_asset(TEST_SWIMMER_PATH)
    generated_class = unreal.BlueprintEditorLibrary.generated_class(blueprint)
    default_object = generated_class.get_default_object() if generated_class else None

    components = []
    if default_object is not None:
        for component in default_object.get_components_by_class(unreal.ActorComponent):
            components.append(
                {
                    "name": component.get_name(),
                    "class": component.get_class().get_name(),
                }
            )

    return {
        "path": blueprint.get_path_name(),
        "generated_class": generated_class.get_name() if generated_class else None,
        "components": components,
    }


def main():
    ensure_output_dir()
    payload = {
        "material": inspect_material(MATERIAL_PATH),
        "ocean_instance": inspect_material(OCEAN_INSTANCE_PATH),
        "test_swimmer": inspect_swimmer_blueprint(),
    }

    with open(OUTPUT_PATH, "w", encoding="utf-8") as handle:
        json.dump(payload, handle, indent=2)

    unreal.log(f"[MYI-70] Wrote inspection data to {OUTPUT_PATH}")


if __name__ == "__main__":
    main()
