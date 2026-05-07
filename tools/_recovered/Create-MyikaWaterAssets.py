import json
from pathlib import Path

import unreal


MATERIAL_DIR = "/Myika/MyikaWater/Materials"
MASTER_NAME = "M_MyikaWater"
NORMAL_TEXTURE_NAME = "T_MyikaWater_Normal"
OCEAN_INSTANCE_PATH = f"{MATERIAL_DIR}/MI_MyikaWater_Ocean"
LAKE_INSTANCE_PATH = f"{MATERIAL_DIR}/MI_MyikaWater_Lake"

OUTPUT_DIR = Path(unreal.Paths.project_saved_dir()) / "Validation" / "MYI-67-water"
OUTPUT_PATH = OUTPUT_DIR / "summary.json"


def ensure_output_dir() -> None:
	OUTPUT_DIR.mkdir(parents=True, exist_ok=True)


def ensure_directory(path: str) -> None:
	if not unreal.EditorAssetLibrary.does_directory_exist(path):
		if not unreal.EditorAssetLibrary.make_directory(path):
			raise RuntimeError(f"Failed to create content directory: {path}")


def save_asset(asset) -> None:
	if not unreal.EditorAssetLibrary.save_loaded_asset(asset, False):
		raise RuntimeError(f"Failed to save asset: {asset.get_path_name()}")


def load_required_asset(asset_path: str):
	asset = unreal.EditorAssetLibrary.load_asset(asset_path)
	if asset is None:
		raise RuntimeError(f"Unable to load asset: {asset_path}")
	return asset


def create_expr(material, expression_class, x, y):
	expression = unreal.MaterialEditingLibrary.create_material_expression(material, expression_class, x, y)
	if expression is None:
		raise RuntimeError(f"Failed to create material expression: {expression_class.__name__}")
	return expression


def connect(from_expression, to_expression, to_input_name: str, from_output_name: str = "") -> None:
	if unreal.MaterialEditingLibrary.connect_material_expressions(
		from_expression, from_output_name, to_expression, to_input_name
	):
		return
	if to_input_name and unreal.MaterialEditingLibrary.connect_material_expressions(
		from_expression, from_output_name, to_expression, ""
	):
		return
	raise RuntimeError(
		f"Failed to connect {from_expression.get_name()}.{from_output_name or 'out'} "
		f"-> {to_expression.get_name()}.{to_input_name}"
	)


def import_normal_texture():
	source_path = Path(__file__).resolve().parent / "Generated" / "MyikaWater" / f"{NORMAL_TEXTURE_NAME}.png"
	if not source_path.exists():
		raise RuntimeError(
			f"Missing generated normal source at {source_path}. "
			"Run tools/generate_myika_water_textures.py first."
		)

	ensure_directory(MATERIAL_DIR)
	texture_asset_path = f"{MATERIAL_DIR}/{NORMAL_TEXTURE_NAME}"

	task = unreal.AssetImportTask()
	task.filename = str(source_path)
	task.destination_path = MATERIAL_DIR
	task.destination_name = NORMAL_TEXTURE_NAME
	task.replace_existing = True
	task.automated = True
	task.save = True

	unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
	texture = load_required_asset(texture_asset_path)
	texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
	texture.set_editor_property("srgb", False)
	save_asset(texture)
	return texture


def make_or_load_master_material():
	asset_path = f"{MATERIAL_DIR}/{MASTER_NAME}"
	if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
		if not unreal.EditorAssetLibrary.delete_asset(asset_path):
			raise RuntimeError(f"Failed to delete prior master material at {asset_path}")
	asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
	material = asset_tools.create_asset(
		MASTER_NAME,
		MATERIAL_DIR,
		unreal.Material,
		unreal.MaterialFactoryNew(),
	)
	if material is None:
		raise RuntimeError(f"Failed to create master material at {asset_path}")
	return material


def build_master_material(normal_texture):
	material = make_or_load_master_material()
	unreal.MaterialEditingLibrary.delete_all_material_expressions(material)

	material.set_editor_property("material_domain", unreal.MaterialDomain.MD_SURFACE)
	material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
	material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_SINGLE_LAYER_WATER)

	water_tint = create_expr(material, unreal.MaterialExpressionVectorParameter, -1200, -600)
	water_tint.set_editor_property("parameter_name", "WaterTint")
	water_tint.set_editor_property("default_value", unreal.LinearColor(0.02, 0.18, 0.28, 1.0))

	roughness = create_expr(material, unreal.MaterialExpressionScalarParameter, -1200, -460)
	roughness.set_editor_property("parameter_name", "Roughness")
	roughness.set_editor_property("default_value", 0.05)

	specular = create_expr(material, unreal.MaterialExpressionScalarParameter, -1200, -380)
	specular.set_editor_property("parameter_name", "Specular")
	specular.set_editor_property("default_value", 0.5)

	metallic = create_expr(material, unreal.MaterialExpressionConstant, -1200, -300)
	metallic.set_editor_property("r", 0.0)

	wave_steepness = create_expr(material, unreal.MaterialExpressionScalarParameter, -1200, -200)
	wave_steepness.set_editor_property("parameter_name", "WaveSteepness")
	wave_steepness.set_editor_property("default_value", 0.4)

	wave_speed = create_expr(material, unreal.MaterialExpressionScalarParameter, -1200, -120)
	wave_speed.set_editor_property("parameter_name", "WaveSpeed")
	wave_speed.set_editor_property("default_value", 0.05)

	wave_dir_x = create_expr(material, unreal.MaterialExpressionScalarParameter, -1200, -40)
	wave_dir_x.set_editor_property("parameter_name", "WaveDirectionX")
	wave_dir_x.set_editor_property("default_value", 1.0)
	wave_dir_y = create_expr(material, unreal.MaterialExpressionScalarParameter, -1200, 0)
	wave_dir_y.set_editor_property("parameter_name", "WaveDirectionY")
	wave_dir_y.set_editor_property("default_value", 0.0)

	wave_dir_xy = create_expr(material, unreal.MaterialExpressionAppendVector, -1000, -20)

	wave_dir_neg_one = create_expr(material, unreal.MaterialExpressionConstant, -1200, 40)
	wave_dir_neg_one.set_editor_property("r", -1.0)
	wave_dir_y_neg = create_expr(material, unreal.MaterialExpressionMultiply, -1000, 40)
	wave_dir_perp = create_expr(material, unreal.MaterialExpressionAppendVector, -800, 20)

	normal_tiling = create_expr(material, unreal.MaterialExpressionScalarParameter, -1200, 90)
	normal_tiling.set_editor_property("parameter_name", "NormalTiling")
	normal_tiling.set_editor_property("default_value", 1.0)

	tile_b_factor = create_expr(material, unreal.MaterialExpressionConstant, -1200, 140)
	tile_b_factor.set_editor_property("r", 1.7)
	tiling_b = create_expr(material, unreal.MaterialExpressionMultiply, -1000, 120)

	tex_coord = create_expr(material, unreal.MaterialExpressionTextureCoordinate, -1200, 180)

	tiled_uv_a = create_expr(material, unreal.MaterialExpressionMultiply, -1000, 180)
	tiled_uv_b = create_expr(material, unreal.MaterialExpressionMultiply, -1000, 280)

	time_node = create_expr(material, unreal.MaterialExpressionTime, -1200, 360)
	time_speed = create_expr(material, unreal.MaterialExpressionMultiply, -1000, 360)
	time_speed_b_factor = create_expr(material, unreal.MaterialExpressionConstant, -1200, 410)
	time_speed_b_factor.set_editor_property("r", 0.7)
	time_speed_b = create_expr(material, unreal.MaterialExpressionMultiply, -1000, 410)

	dir_offset_a = create_expr(material, unreal.MaterialExpressionMultiply, -800, 200)
	dir_offset_b = create_expr(material, unreal.MaterialExpressionMultiply, -800, 300)
	shifted_uv_a = create_expr(material, unreal.MaterialExpressionAdd, -700, 180)
	shifted_uv_b = create_expr(material, unreal.MaterialExpressionAdd, -700, 280)

	normal_sample_a = create_expr(material, unreal.MaterialExpressionTextureSampleParameter2D, -550, 180)
	normal_sample_a.set_editor_property("parameter_name", "WaterNormalA")
	normal_sample_a.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
	normal_sample_a.texture = normal_texture

	normal_sample_b = create_expr(material, unreal.MaterialExpressionTextureSampleParameter2D, -550, 320)
	normal_sample_b.set_editor_property("parameter_name", "WaterNormalB")
	normal_sample_b.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
	normal_sample_b.texture = normal_texture

	normal_lerp = create_expr(material, unreal.MaterialExpressionLinearInterpolate, -300, 250)
	normal_lerp_alpha = create_expr(material, unreal.MaterialExpressionConstant, -550, 460)
	normal_lerp_alpha.set_editor_property("r", 0.5)

	flat_normal = create_expr(material, unreal.MaterialExpressionConstant3Vector, -300, 380)
	flat_normal.set_editor_property("constant", unreal.LinearColor(0.5, 0.5, 1.0, 0.0))

	normal_blend = create_expr(material, unreal.MaterialExpressionLinearInterpolate, -80, 320)

	scattering_color = create_expr(material, unreal.MaterialExpressionVectorParameter, -1200, 540)
	scattering_color.set_editor_property("parameter_name", "ScatteringColor")
	scattering_color.set_editor_property("default_value", unreal.LinearColor(0.4, 0.55, 0.5, 1.0))

	absorption_color = create_expr(material, unreal.MaterialExpressionVectorParameter, -1200, 620)
	absorption_color.set_editor_property("parameter_name", "AbsorptionColor")
	absorption_color.set_editor_property("default_value", unreal.LinearColor(0.6, 0.4, 0.2, 1.0))

	extinction_scale = create_expr(material, unreal.MaterialExpressionScalarParameter, -1200, 700)
	extinction_scale.set_editor_property("parameter_name", "ExtinctionScale")
	extinction_scale.set_editor_property("default_value", 1.0)

	scattering_scaled = create_expr(material, unreal.MaterialExpressionMultiply, -900, 540)
	absorption_scaled = create_expr(material, unreal.MaterialExpressionMultiply, -900, 620)

	water_albedo = create_expr(material, unreal.MaterialExpressionScalarParameter, -1200, 780)
	water_albedo.set_editor_property("parameter_name", "WaterAlbedo")
	water_albedo.set_editor_property("default_value", 0.05)

	water_phase_g = create_expr(material, unreal.MaterialExpressionScalarParameter, -1200, 840)
	water_phase_g.set_editor_property("parameter_name", "WaterPhaseG")
	water_phase_g.set_editor_property("default_value", 0.0)

	slw_output = create_expr(material, unreal.MaterialExpressionSingleLayerWaterMaterialOutput, 400, 600)

	connect(normal_tiling, tiling_b, "A")
	connect(tile_b_factor, tiling_b, "B")

	connect(tex_coord, tiled_uv_a, "A")
	connect(normal_tiling, tiled_uv_a, "B")
	connect(tex_coord, tiled_uv_b, "A")
	connect(tiling_b, tiled_uv_b, "B")

	connect(wave_dir_x, wave_dir_xy, "A")
	connect(wave_dir_y, wave_dir_xy, "B")
	connect(wave_dir_y, wave_dir_y_neg, "A")
	connect(wave_dir_neg_one, wave_dir_y_neg, "B")
	connect(wave_dir_y_neg, wave_dir_perp, "A")
	connect(wave_dir_x, wave_dir_perp, "B")

	connect(time_node, time_speed, "A")
	connect(wave_speed, time_speed, "B")
	connect(time_speed, time_speed_b, "A")
	connect(time_speed_b_factor, time_speed_b, "B")

	connect(wave_dir_xy, dir_offset_a, "A")
	connect(time_speed, dir_offset_a, "B")
	connect(wave_dir_perp, dir_offset_b, "A")
	connect(time_speed_b, dir_offset_b, "B")

	connect(tiled_uv_a, shifted_uv_a, "A")
	connect(dir_offset_a, shifted_uv_a, "B")
	connect(tiled_uv_b, shifted_uv_b, "A")
	connect(dir_offset_b, shifted_uv_b, "B")

	connect(shifted_uv_a, normal_sample_a, "Coordinates")
	connect(shifted_uv_b, normal_sample_b, "Coordinates")

	connect(normal_sample_a, normal_lerp, "A")
	connect(normal_sample_b, normal_lerp, "B")
	connect(normal_lerp_alpha, normal_lerp, "Alpha")

	connect(flat_normal, normal_blend, "A")
	connect(normal_lerp, normal_blend, "B")
	connect(wave_steepness, normal_blend, "Alpha")

	connect(scattering_color, scattering_scaled, "A")
	connect(extinction_scale, scattering_scaled, "B")
	connect(absorption_color, absorption_scaled, "A")
	connect(extinction_scale, absorption_scaled, "B")

	if not unreal.MaterialEditingLibrary.connect_material_property(
		water_tint, "", unreal.MaterialProperty.MP_BASE_COLOR
	):
		raise RuntimeError("Failed to connect WaterTint -> Base Color")
	if not unreal.MaterialEditingLibrary.connect_material_property(
		metallic, "", unreal.MaterialProperty.MP_METALLIC
	):
		raise RuntimeError("Failed to connect Metallic")
	if not unreal.MaterialEditingLibrary.connect_material_property(
		specular, "", unreal.MaterialProperty.MP_SPECULAR
	):
		raise RuntimeError("Failed to connect Specular")
	if not unreal.MaterialEditingLibrary.connect_material_property(
		roughness, "", unreal.MaterialProperty.MP_ROUGHNESS
	):
		raise RuntimeError("Failed to connect Roughness")
	if not unreal.MaterialEditingLibrary.connect_material_property(
		normal_blend, "", unreal.MaterialProperty.MP_NORMAL
	):
		raise RuntimeError("Failed to connect Normal")

	connect(scattering_scaled, slw_output, "ScatteringCoefficients")
	connect(absorption_scaled, slw_output, "AbsorptionCoefficients")
	connect(water_albedo, slw_output, "WaterAlbedo")
	connect(water_phase_g, slw_output, "WaterPhaseG")

	unreal.MaterialEditingLibrary.layout_material_expressions(material)
	recompile_ok = unreal.MaterialEditingLibrary.recompile_material(material)
	if not recompile_ok:
		# Patched 2026-05-05 (Claude, Myika lane): the original implementation only logged a
		# warning here. That swallowed the May-3 SLW shader-compile failure and let the queue
		# task `2026-05-03-026-myi67-fix-water-master-material.md` get marked complete with
		# `recompile_ok: false` in MYI-67-water/summary.json. MYI-67 was bounced back from
		# In Review on 2026-05-05 because the four script-generated assets were rolled back
		# from disk and never compiled cleanly. Failing loud now so we don't repeat the bug.
		raise RuntimeError(
			f"[MyikaWater] recompile_material returned False for {material.get_path_name()}. "
			"Master material did not compile; refusing to proceed. Check OutputLog for the "
			"underlying SM6/SM5 error and re-run after fixing the graph."
		)
	save_asset(material)
	return material, recompile_ok


def reparent_instance(instance_path: str, master_material, overrides: dict) -> None:
	if not unreal.EditorAssetLibrary.does_asset_exist(instance_path):
		asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
		instance = asset_tools.create_asset(
			Path(instance_path).name,
			str(Path(instance_path).parent).replace("\\", "/"),
			unreal.MaterialInstanceConstant,
			unreal.MaterialInstanceConstantFactoryNew(),
		)
		if instance is None:
			raise RuntimeError(f"Failed to create material instance: {instance_path}")
	else:
		instance = load_required_asset(instance_path)

	unreal.MaterialEditingLibrary.set_material_instance_parent(instance, master_material)

	for name, value in overrides.get("scalars", {}).items():
		unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(instance, name, float(value))
	for name, value in overrides.get("vectors", {}).items():
		unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(instance, name, value)

	save_asset(instance)


def write_summary(material, recompile_ok: bool) -> None:
	# Patched 2026-05-05: status now reflects recompile outcome instead of always being "ok".
	# Previously this could write `{status: ok, recompile_ok: false}` and downstream consumers
	# treated the run as a success despite the broken shader.
	data = {
		"status": "ok" if recompile_ok else "fail",
		"recompile_ok": bool(recompile_ok),
		"material_path": material.get_path_name(),
		"shading_model": str(material.get_editor_property("shading_model")),
		"material_domain": str(material.get_editor_property("material_domain")),
		"blend_mode": str(material.get_editor_property("blend_mode")),
		"ocean_instance": OCEAN_INSTANCE_PATH,
		"lake_instance": LAKE_INSTANCE_PATH,
		"normal_texture": f"{MATERIAL_DIR}/{NORMAL_TEXTURE_NAME}",
	}
	OUTPUT_PATH.write_text(json.dumps(data, indent=2), encoding="utf-8")


def main():
	ensure_output_dir()
	ensure_directory(MATERIAL_DIR)
	normal_texture = import_normal_texture()
	master, recompile_ok = build_master_material(normal_texture)

	reparent_instance(
		OCEAN_INSTANCE_PATH,
		master,
		{
			"scalars": {
				"WaveSteepness": 0.6,
				"WaveSpeed": 0.08,
				"NormalTiling": 0.5,
				"ExtinctionScale": 1.0,
				"Roughness": 0.04,
			},
			"vectors": {
				"WaterTint": unreal.LinearColor(0.01, 0.12, 0.22, 1.0),
				"ScatteringColor": unreal.LinearColor(0.30, 0.45, 0.45, 1.0),
				"AbsorptionColor": unreal.LinearColor(0.55, 0.30, 0.15, 1.0),
			},
		},
	)
	reparent_instance(
		LAKE_INSTANCE_PATH,
		master,
		{
			"scalars": {
				"WaveSteepness": 0.2,
				"WaveSpeed": 0.03,
				"NormalTiling": 1.5,
				"ExtinctionScale": 0.5,
				"Roughness": 0.08,
			},
			"vectors": {
				"WaterTint": unreal.LinearColor(0.05, 0.20, 0.18, 1.0),
				"ScatteringColor": unreal.LinearColor(0.45, 0.55, 0.40, 1.0),
				"AbsorptionColor": unreal.LinearColor(0.45, 0.30, 0.20, 1.0),
			},
		},
	)

	write_summary(master, recompile_ok)
	unreal.log(f"[MyikaWater] Built {master.get_path_name()} (recompile_ok={recompile_ok})")


if __name__ == "__main__":
	main()
