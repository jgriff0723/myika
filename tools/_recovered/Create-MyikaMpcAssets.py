import json
import os

import unreal


MPC_DIR = "/Myika/MyikaCore"
DEBUG_DIR = "/Myika/MyikaCore/Debug"
MPC_NAME = "MPC_Myika"
DEBUG_MATERIAL_NAME = "M_MPCDebug"
MPC_PATH = f"{MPC_DIR}/{MPC_NAME}"
DEBUG_MATERIAL_PATH = f"{DEBUG_DIR}/{DEBUG_MATERIAL_NAME}"
MPC_OBJECT_PATH = f"{MPC_PATH}.{MPC_NAME}"
DEBUG_MATERIAL_OBJECT_PATH = f"{DEBUG_MATERIAL_PATH}.{DEBUG_MATERIAL_NAME}"
OUTPUT_DIR = os.path.join(unreal.Paths.project_saved_dir(), "Validation", "MYI-54-mpc")
OUTPUT_PATH = os.path.join(OUTPUT_DIR, "generation.json")

SCALAR_PARAMETERS = [
    ("TimeOfDay", 12.0),
    ("Wetness", 0.0),
    ("SnowCoverage", 0.0),
    ("Temperature", 20.0),
    ("LightningFlash", 0.0),
    ("StormIntensity", 0.0),
    ("WaterLevel", 0.0),
]

VECTOR_PARAMETERS = [
    ("SunDirection", unreal.LinearColor(0.0, 0.0, -1.0, 0.0)),
    ("MoonDirection", unreal.LinearColor(0.0, 0.0, 1.0, 0.0)),
    ("WindVector", unreal.LinearColor(0.0, 0.0, 0.0, 0.0)),
]


def ensure_output_dir():
    os.makedirs(OUTPUT_DIR, exist_ok=True)


def ensure_directory(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        if not unreal.EditorAssetLibrary.make_directory(path):
            raise RuntimeError(f"Failed to create content directory: {path}")


def delete_asset_if_present(asset_path):
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        if not unreal.EditorAssetLibrary.delete_asset(asset_path):
            raise RuntimeError(f"Failed to delete existing asset: {asset_path}")


def load_existing_asset(package_path, object_path):
    asset = unreal.EditorAssetLibrary.load_asset(package_path)
    if asset is not None:
        return asset

    asset = unreal.load_asset(package_path)
    if asset is not None:
        return asset

    asset = unreal.load_asset(object_path)
    if asset is not None:
        return asset

    return None


def save_asset(asset):
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, False):
        raise RuntimeError(f"Failed to save asset: {asset.get_path_name()}")


def make_scalar_parameter(name, default_value):
    parameter = unreal.CollectionScalarParameter()
    parameter.set_editor_property("parameter_name", name)
    parameter.set_editor_property("default_value", default_value)
    return parameter


def make_vector_parameter(name, default_value):
    parameter = unreal.CollectionVectorParameter()
    parameter.set_editor_property("parameter_name", name)
    parameter.set_editor_property("default_value", default_value)
    return parameter


def create_material_expression(material, expression_class, x, y):
    expression = unreal.MaterialEditingLibrary.create_material_expression(material, expression_class, x, y)
    if expression is None:
        raise RuntimeError(f"Failed to create material expression: {expression_class.__name__}")
    return expression


def connect_expression_output(from_expression, to_expression, to_input_name):
    if not unreal.MaterialEditingLibrary.connect_material_expressions(from_expression, "", to_expression, to_input_name):
        raise RuntimeError(
            f"Failed to connect {from_expression.get_name()} -> {to_expression.get_name()}.{to_input_name}"
        )


def create_collection():
    ensure_directory(MPC_DIR)
    collection = load_existing_asset(MPC_PATH, MPC_OBJECT_PATH)
    if collection is None:
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        collection = asset_tools.create_asset(
            MPC_NAME,
            MPC_DIR,
            unreal.MaterialParameterCollection,
            unreal.MaterialParameterCollectionFactoryNew(),
        )
        if collection is None:
            raise RuntimeError("Failed to create MPC_Myika asset")

    scalar_parameters = [make_scalar_parameter(name, default_value) for name, default_value in SCALAR_PARAMETERS]
    vector_parameters = [make_vector_parameter(name, default_value) for name, default_value in VECTOR_PARAMETERS]

    collection.set_editor_property("scalar_parameters", scalar_parameters)
    collection.set_editor_property("vector_parameters", vector_parameters)
    save_asset(collection)

    return collection


def create_debug_material(collection):
    ensure_directory(DEBUG_DIR)
    material = load_existing_asset(DEBUG_MATERIAL_PATH, DEBUG_MATERIAL_OBJECT_PATH)
    if material is None:
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        material = asset_tools.create_asset(
            DEBUG_MATERIAL_NAME,
            DEBUG_DIR,
            unreal.Material,
            unreal.MaterialFactoryNew(),
        )
        if material is None:
            raise RuntimeError("Failed to create M_MPCDebug asset")

    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)

    tex_coord = create_material_expression(material, unreal.MaterialExpressionTextureCoordinate, -1600, 0)

    u_mask = create_material_expression(material, unreal.MaterialExpressionComponentMask, -1350, -200)
    u_mask.set_editor_property("r", True)
    u_mask.set_editor_property("g", False)
    u_mask.set_editor_property("b", False)
    u_mask.set_editor_property("a", False)
    connect_expression_output(tex_coord, u_mask, "")

    v_mask = create_material_expression(material, unreal.MaterialExpressionComponentMask, -1350, 200)
    v_mask.set_editor_property("r", False)
    v_mask.set_editor_property("g", True)
    v_mask.set_editor_property("b", False)
    v_mask.set_editor_property("a", False)
    connect_expression_output(tex_coord, v_mask, "")

    time_param = create_material_expression(material, unreal.MaterialExpressionCollectionParameter, -1350, -500)
    time_param.set_editor_property("collection", collection)
    time_param.set_editor_property("parameter_name", "TimeOfDay")

    wetness_param = create_material_expression(material, unreal.MaterialExpressionCollectionParameter, -1350, 500)
    wetness_param.set_editor_property("collection", collection)
    wetness_param.set_editor_property("parameter_name", "Wetness")

    const_24 = create_material_expression(material, unreal.MaterialExpressionConstant, -1150, -650)
    const_24.set_editor_property("r", 24.0)

    const_bar_scale = create_material_expression(material, unreal.MaterialExpressionConstant, -900, 0)
    const_bar_scale.set_editor_property("r", 10000.0)

    time_normalized = create_material_expression(material, unreal.MaterialExpressionDivide, -1050, -500)
    connect_expression_output(time_param, time_normalized, "")
    connect_expression_output(const_24, time_normalized, "B")

    time_delta = create_material_expression(material, unreal.MaterialExpressionSubtract, -850, -250)
    connect_expression_output(time_normalized, time_delta, "")
    connect_expression_output(u_mask, time_delta, "B")

    time_scaled = create_material_expression(material, unreal.MaterialExpressionMultiply, -650, -250)
    connect_expression_output(time_delta, time_scaled, "")
    connect_expression_output(const_bar_scale, time_scaled, "B")

    time_bar = create_material_expression(material, unreal.MaterialExpressionSaturate, -450, -250)
    connect_expression_output(time_scaled, time_bar, "")

    wetness_delta = create_material_expression(material, unreal.MaterialExpressionSubtract, -850, 250)
    connect_expression_output(wetness_param, wetness_delta, "")
    connect_expression_output(v_mask, wetness_delta, "B")

    wetness_scaled = create_material_expression(material, unreal.MaterialExpressionMultiply, -650, 250)
    connect_expression_output(wetness_delta, wetness_scaled, "")
    connect_expression_output(const_bar_scale, wetness_scaled, "B")

    wetness_bar = create_material_expression(material, unreal.MaterialExpressionSaturate, -450, 250)
    connect_expression_output(wetness_scaled, wetness_bar, "")

    red = create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -500, -400)
    red.set_editor_property("constant", unreal.LinearColor(1.0, 0.0, 0.0, 1.0))

    blue = create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -500, 400)
    blue.set_editor_property("constant", unreal.LinearColor(0.0, 0.0, 1.0, 1.0))

    red_bar_color = create_material_expression(material, unreal.MaterialExpressionMultiply, -250, -250)
    connect_expression_output(time_bar, red_bar_color, "")
    connect_expression_output(red, red_bar_color, "B")

    blue_bar_color = create_material_expression(material, unreal.MaterialExpressionMultiply, -250, 250)
    connect_expression_output(wetness_bar, blue_bar_color, "")
    connect_expression_output(blue, blue_bar_color, "B")

    combined_color = create_material_expression(material, unreal.MaterialExpressionAdd, -100, 0)
    connect_expression_output(red_bar_color, combined_color, "")
    connect_expression_output(blue_bar_color, combined_color, "B")

    if not unreal.MaterialEditingLibrary.connect_material_property(
        combined_color, "", unreal.MaterialProperty.MP_BASE_COLOR
    ):
        raise RuntimeError("Failed to connect debug color to Base Color")

    if not unreal.MaterialEditingLibrary.connect_material_property(
        combined_color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    ):
        raise RuntimeError("Failed to connect debug color to Emissive Color")

    unreal.MaterialEditingLibrary.layout_material_expressions(material)
    unreal.MaterialEditingLibrary.recompile_material(material)
    save_asset(material)

    return material


def write_summary(collection, material):
    data = {
        "status": "ok",
        "collection_path": collection.get_path_name(),
        "material_path": material.get_path_name(),
        "scalar_parameters": [
            {
                "name": str(parameter.get_editor_property("parameter_name")),
                "default": parameter.get_editor_property("default_value"),
            }
            for parameter in collection.get_editor_property("scalar_parameters")
        ],
        "vector_parameters": [
            {
                "name": str(parameter.get_editor_property("parameter_name")),
                "default": {
                    "r": parameter.get_editor_property("default_value").r,
                    "g": parameter.get_editor_property("default_value").g,
                    "b": parameter.get_editor_property("default_value").b,
                    "a": parameter.get_editor_property("default_value").a,
                },
            }
            for parameter in collection.get_editor_property("vector_parameters")
        ],
    }

    with open(OUTPUT_PATH, "w", encoding="utf-8") as handle:
        json.dump(data, handle, indent=2)


def main():
    ensure_output_dir()
    collection = create_collection()
    material = create_debug_material(collection)
    write_summary(collection, material)
    unreal.log(f"Generated {collection.get_path_name()} and {material.get_path_name()}")


if __name__ == "__main__":
    main()
