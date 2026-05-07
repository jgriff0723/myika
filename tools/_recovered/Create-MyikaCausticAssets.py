import os
import json
from pathlib import Path

import unreal


DECAL_DIR = "/Myika/MyikaWater/Decals"
TEXTURE_NAME = "T_MyikaCaustic_Flipbook"
MATERIAL_NAME = "M_MyikaCaustic"
MPC_PATH = "/Myika/MyikaCore/MPC_Myika"
OUTPUT_DIR = Path(unreal.Paths.project_saved_dir()) / "Validation" / "MYI-72-caustics"
OUTPUT_PATH = OUTPUT_DIR / "generation.json"


def ensure_directory(path: str) -> None:
	if not unreal.EditorAssetLibrary.does_directory_exist(path):
		if not unreal.EditorAssetLibrary.make_directory(path):
			raise RuntimeError(f"Failed to create content directory: {path}")


def delete_asset_if_present(asset_path: str) -> None:
	if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
		if not unreal.EditorAssetLibrary.delete_asset(asset_path):
			raise RuntimeError(f"Failed to delete existing asset: {asset_path}")


def save_asset(asset) -> None:
	if not unreal.EditorAssetLibrary.save_loaded_asset(asset, False):
		raise RuntimeError(f"Failed to save asset: {asset.get_path_name()}")


def create_material_expression(material, expression_class, x, y):
	expression = unreal.MaterialEditingLibrary.create_material_expression(material, expression_class, x, y)
	if expression is None:
		raise RuntimeError(f"Failed to create material expression: {expression_class.__name__}")
	return expression


def mpc_has_vector_parameter(collection, parameter_name: str) -> bool:
	for parameter in collection.get_editor_property("vector_parameters"):
		if str(parameter.get_editor_property("parameter_name")) == parameter_name:
			return True
	return False


def connect_expression_output(from_expression, to_expression, to_input_name: str) -> None:
	if unreal.MaterialEditingLibrary.connect_material_expressions(from_expression, "", to_expression, to_input_name):
		return

	if to_input_name and unreal.MaterialEditingLibrary.connect_material_expressions(from_expression, "", to_expression, ""):
		return

	raise RuntimeError(
		f"Failed to connect {from_expression.get_name()} -> {to_expression.get_name()}.{to_input_name}"
	)


def load_required_asset(asset_path: str):
	asset = unreal.EditorAssetLibrary.load_asset(asset_path)
	if asset is None:
		raise RuntimeError(f"Unable to load asset: {asset_path}")
	return asset


def ensure_generated_source() -> Path:
	source_path = Path(__file__).resolve().parent / "Generated" / "MyikaWater" / f"{TEXTURE_NAME}.png"
	if not source_path.exists():
		raise RuntimeError(
			f"Missing generated source atlas at {source_path}. "
			"Run tools/generate_myika_caustic_source.py first."
		)
	return source_path


def ensure_output_dir() -> None:
	OUTPUT_DIR.mkdir(parents=True, exist_ok=True)


def import_texture(source_path: Path):
	ensure_directory(DECAL_DIR)

	texture_asset_path = f"{DECAL_DIR}/{TEXTURE_NAME}"

	task = unreal.AssetImportTask()
	task.filename = str(source_path)
	task.destination_path = DECAL_DIR
	task.destination_name = TEXTURE_NAME
	task.replace_existing = True
	task.automated = True
	task.save = True

	unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
	texture = load_required_asset(texture_asset_path)
	texture.set_editor_property("srgb", False)
	save_asset(texture)
	return texture


def build_material(texture, collection):
	material_asset_path = f"{DECAL_DIR}/{MATERIAL_NAME}"
	if unreal.EditorAssetLibrary.does_asset_exist(material_asset_path):
		material = load_required_asset(material_asset_path)
	else:
		asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
		material = asset_tools.create_asset(
			MATERIAL_NAME,
			DECAL_DIR,
			unreal.Material,
			unreal.MaterialFactoryNew(),
		)
		if material is None:
			raise RuntimeError(f"Failed to create material asset: {material_asset_path}")

	unreal.MaterialEditingLibrary.delete_all_material_expressions(material)

	material.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
	material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)

	sun_drift_enabled = mpc_has_vector_parameter(collection, "SunDirection")
	if not sun_drift_enabled:
		unreal.log_warning(
			"[MyikaCaustic] MPC_Myika has no Vector parameter 'SunDirection'; "
			"building caustic without sun-drift wires."
		)

	tex_coord = create_material_expression(material, unreal.MaterialExpressionTextureCoordinate, -1500, -200)
	time_node = create_material_expression(material, unreal.MaterialExpressionTime, -1500, 80)
	speed_param = create_material_expression(material, unreal.MaterialExpressionScalarParameter, -1500, 220)
	speed_param.set_editor_property("parameter_name", "AnimationSpeed")
	speed_param.set_editor_property("default_value", 1.5)

	frame_count = create_material_expression(material, unreal.MaterialExpressionConstant, -1450, 380)
	frame_count.set_editor_property("r", 16.0)
	cols = create_material_expression(material, unreal.MaterialExpressionConstant, -1250, 380)
	cols.set_editor_property("r", 4.0)
	rows = create_material_expression(material, unreal.MaterialExpressionConstant, -1250, 460)
	rows.set_editor_property("r", 4.0)

	mul_time_speed = create_material_expression(material, unreal.MaterialExpressionMultiply, -1280, 100)
	floor_frame = create_material_expression(material, unreal.MaterialExpressionFloor, -1090, 100)
	frame_mod = create_material_expression(material, unreal.MaterialExpressionFmod, -910, 100)
	col_index = create_material_expression(material, unreal.MaterialExpressionFmod, -730, 40)
	row_divide = create_material_expression(material, unreal.MaterialExpressionDivide, -730, 160)
	row_index = create_material_expression(material, unreal.MaterialExpressionFloor, -550, 160)

	col_offset = create_material_expression(material, unreal.MaterialExpressionDivide, -550, 40)
	row_offset = create_material_expression(material, unreal.MaterialExpressionDivide, -390, 160)
	tile_offset = create_material_expression(material, unreal.MaterialExpressionAppendVector, -210, 100)

	grid_size = create_material_expression(material, unreal.MaterialExpressionAppendVector, -1080, -170)
	base_uv = create_material_expression(material, unreal.MaterialExpressionDivide, -880, -180)
	flipbook_uv = create_material_expression(material, unreal.MaterialExpressionAdd, 10, -80)

	if sun_drift_enabled:
		sun_direction = create_material_expression(material, unreal.MaterialExpressionCollectionParameter, -760, -450)
		sun_direction.set_editor_property("collection", collection)
		sun_direction.set_editor_property("parameter_name", "SunDirection")

		sun_x = create_material_expression(material, unreal.MaterialExpressionComponentMask, -580, -510)
		sun_x.set_editor_property("r", True)
		sun_y = create_material_expression(material, unreal.MaterialExpressionComponentMask, -580, -420)
		sun_y.set_editor_property("g", True)
		sun_xy = create_material_expression(material, unreal.MaterialExpressionAppendVector, -400, -470)

		drift_strength = create_material_expression(material, unreal.MaterialExpressionScalarParameter, -580, -290)
		drift_strength.set_editor_property("parameter_name", "SunDriftStrength")
		drift_strength.set_editor_property("default_value", 1.0)
		drift_scale = create_material_expression(material, unreal.MaterialExpressionConstant, -390, -290)
		drift_scale.set_editor_property("r", 0.04)
		drift_mul = create_material_expression(material, unreal.MaterialExpressionMultiply, -210, -390)
		drift_final = create_material_expression(material, unreal.MaterialExpressionMultiply, -20, -390)
		drifted_uv = create_material_expression(material, unreal.MaterialExpressionAdd, 220, -160)

	texture_sample = create_material_expression(material, unreal.MaterialExpressionTextureSampleParameter2D, 450, -120)
	texture_sample.set_editor_property("parameter_name", "CausticFlipbook")
	texture_sample.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
	texture_sample.texture = texture

	tint = create_material_expression(material, unreal.MaterialExpressionVectorParameter, 700, -270)
	tint.set_editor_property("parameter_name", "CausticTint")
	tint.set_editor_property("default_value", unreal.LinearColor(0.62, 0.90, 1.0, 1.0))

	intensity = create_material_expression(material, unreal.MaterialExpressionScalarParameter, 700, 40)
	intensity.set_editor_property("parameter_name", "CausticIntensity")
	intensity.set_editor_property("default_value", 1.0)

	alpha_mask = create_material_expression(material, unreal.MaterialExpressionComponentMask, 700, 150)
	alpha_mask.set_editor_property("r", True)
	alpha_power = create_material_expression(material, unreal.MaterialExpressionPower, 900, 150)
	alpha_sharpness = create_material_expression(material, unreal.MaterialExpressionConstant, 700, 250)
	alpha_sharpness.set_editor_property("r", 1.6)
	alpha_intensity = create_material_expression(material, unreal.MaterialExpressionMultiply, 1080, 150)

	tinted_color = create_material_expression(material, unreal.MaterialExpressionMultiply, 900, -90)
	emissive_color = create_material_expression(material, unreal.MaterialExpressionMultiply, 1110, -90)
	emissive_scale = create_material_expression(material, unreal.MaterialExpressionConstant, 930, 10)
	emissive_scale.set_editor_property("r", 0.35)

	connect_expression_output(time_node, mul_time_speed, "A")
	connect_expression_output(speed_param, mul_time_speed, "B")
	connect_expression_output(mul_time_speed, floor_frame, "Input")
	connect_expression_output(floor_frame, frame_mod, "A")
	connect_expression_output(frame_count, frame_mod, "B")
	connect_expression_output(frame_mod, col_index, "A")
	connect_expression_output(cols, col_index, "B")
	connect_expression_output(frame_mod, row_divide, "A")
	connect_expression_output(cols, row_divide, "B")
	connect_expression_output(row_divide, row_index, "Input")
	connect_expression_output(col_index, col_offset, "A")
	connect_expression_output(cols, col_offset, "B")
	connect_expression_output(row_index, row_offset, "A")
	connect_expression_output(rows, row_offset, "B")
	connect_expression_output(col_offset, tile_offset, "A")
	connect_expression_output(row_offset, tile_offset, "B")

	connect_expression_output(cols, grid_size, "A")
	connect_expression_output(rows, grid_size, "B")
	connect_expression_output(tex_coord, base_uv, "A")
	connect_expression_output(grid_size, base_uv, "B")
	connect_expression_output(base_uv, flipbook_uv, "A")
	connect_expression_output(tile_offset, flipbook_uv, "B")

	if sun_drift_enabled:
		connect_expression_output(sun_direction, sun_x, "Input")
		connect_expression_output(sun_direction, sun_y, "Input")
		connect_expression_output(sun_x, sun_xy, "A")
		connect_expression_output(sun_y, sun_xy, "B")
		connect_expression_output(sun_xy, drift_mul, "A")
		connect_expression_output(drift_strength, drift_mul, "B")
		connect_expression_output(drift_mul, drift_final, "A")
		connect_expression_output(drift_scale, drift_final, "B")
		connect_expression_output(flipbook_uv, drifted_uv, "A")
		connect_expression_output(drift_final, drifted_uv, "B")
		connect_expression_output(drifted_uv, texture_sample, "Coordinates")
	else:
		connect_expression_output(flipbook_uv, texture_sample, "Coordinates")

	connect_expression_output(texture_sample, tinted_color, "A")
	connect_expression_output(tint, tinted_color, "B")
	connect_expression_output(tinted_color, emissive_color, "A")
	connect_expression_output(emissive_scale, emissive_color, "B")

	connect_expression_output(texture_sample, alpha_mask, "Input")
	connect_expression_output(alpha_mask, alpha_power, "Base")
	connect_expression_output(alpha_sharpness, alpha_power, "Exp")
	connect_expression_output(alpha_power, alpha_intensity, "A")
	connect_expression_output(intensity, alpha_intensity, "B")

	if not unreal.MaterialEditingLibrary.connect_material_property(
		tinted_color, "", unreal.MaterialProperty.MP_BASE_COLOR
	):
		raise RuntimeError("Failed to connect caustic color to Base Color")

	if not unreal.MaterialEditingLibrary.connect_material_property(
		alpha_intensity, "", unreal.MaterialProperty.MP_OPACITY
	):
		raise RuntimeError("Failed to connect caustic opacity")

	if not unreal.MaterialEditingLibrary.connect_material_property(
		emissive_color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
	):
		raise RuntimeError("Failed to connect caustic emissive color")

	unreal.MaterialEditingLibrary.layout_material_expressions(material)
	if not unreal.MaterialEditingLibrary.recompile_material(material):
		unreal.log_warning(
			f"[MyikaCaustic] recompile_material returned False for {material.get_path_name()} "
			"— may be benign for a fresh asset; verify in editor."
		)
	save_asset(material)
	return material


def enum_name(value) -> str:
	try:
		return value.name
	except Exception:
		return str(value)


def write_summary(texture, material) -> None:
	texture_asset = load_required_asset(texture.get_path_name())
	material_asset = load_required_asset(material.get_path_name())

	data = {
		"status": "ok",
		"texture_path": texture_asset.get_path_name(),
		"material_path": material_asset.get_path_name(),
		"material_domain": enum_name(material_asset.get_editor_property("material_domain")),
		"blend_mode": enum_name(material_asset.get_editor_property("blend_mode")),
		"texture_srgb": texture_asset.get_editor_property("srgb"),
	}

	OUTPUT_PATH.write_text(json.dumps(data, indent=2), encoding="utf-8")


def main():
	ensure_output_dir()
	source_path = ensure_generated_source()
	texture = import_texture(source_path)
	collection = load_required_asset(MPC_PATH)
	material = build_material(texture, collection)
	write_summary(texture, material)
	unreal.log(f"Generated {texture.get_path_name()} and {material.get_path_name()}")


if __name__ == "__main__":
	main()
