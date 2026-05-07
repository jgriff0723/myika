import json
from pathlib import Path

import unreal


CAUSTIC_PATH = "/Myika/MyikaWater/Decals/M_MyikaCaustic"
WATER_PATH = "/Myika/MyikaWater/Materials/M_MyikaWater"
OCEAN_PATH = "/Myika/MyikaWater/Materials/MI_MyikaWater_Ocean"
LAKE_PATH = "/Myika/MyikaWater/Materials/MI_MyikaWater_Lake"
OUTPUT_PATH = Path(unreal.Paths.project_saved_dir()) / "Validation" / "MYI-67-72-verify" / "verify.json"


def check(asset_path: str) -> dict:
	asset = unreal.EditorAssetLibrary.load_asset(asset_path)
	if asset is None:
		return {"path": asset_path, "loaded": False}
	out = {"path": asset_path, "loaded": True, "class": asset.get_class().get_name()}
	if isinstance(asset, unreal.MaterialInterface):
		try:
			recompile = unreal.MaterialEditingLibrary.recompile_material(
				asset if isinstance(asset, unreal.Material) else asset.get_base_material()
			)
			out["recompile_ok"] = bool(recompile)
		except Exception as exc:
			out["recompile_error"] = str(exc)
	return out


def main():
	OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
	results = [check(p) for p in (WATER_PATH, OCEAN_PATH, LAKE_PATH, CAUSTIC_PATH)]
	OUTPUT_PATH.write_text(json.dumps({"results": results}, indent=2), encoding="utf-8")
	for r in results:
		unreal.log(f"[MyikaVerify] {r}")


if __name__ == "__main__":
	main()
