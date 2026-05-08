"""
Author the 6 UMyikaCloudPreset UPrimaryDataAsset instances at
/Myika/MyikaSky/Presets/Clouds/ that AMyikaSkyActor.LerpToPreset is built
to consume.

Split off 2026-05-05 from tools/_recovered/python/create_missing_assets.py
so the cloud-preset author path is testable in isolation. The original
script also authored M_MyikaUnderwaterPP (which already exists on disk
post-quarantine) and ran in the same session as M_MyikaStars +
M_MyikaMoon — when L_SkyTest then crashed, the diagnosis blamed
"presets + curves + celestial materials" together. Static review
2026-05-05 showed the cloud presets are UPrimaryDataAssets that don't
compile shaders and are therefore innocent. Running this script alone
should not reproduce the May-3 editor hang.

Run via:
    UnrealEditor-Cmd.exe <project> -unattended ^
        -ExecCmds="py <this-file>" -log -abslog=<log>

Recommended verification:
    1. Run this script.
    2. Open L_SkyTest, walk PIE for ~30 seconds.
    3. If editor remains stable: cloud presets are confirmed innocent;
       MYI-60 PIE-passes once LerpToPreset is exercised against any preset.
    4. If editor hangs/crashes: revert the 6 presets to confirm the crash
       follows them (unlikely but verify).

Linear: MYI-60 (V1-S5b).
Queue: pending/2026-05-05-001-myi60-cloud-presets-reauthor.md
"""

import json
import traceback
from pathlib import Path

import unreal


PRESETS_FOLDER = "/Myika/MyikaSky/Presets/Clouds"

# (asset_name, coverage, morphology, wind_strength, extinction, detail,
#  wind_dir_xyz, base_color_rgba)
PRESETS = [
    ("Preset_Clear",        0.05, 0.30, 0.20, 0.40, 0.30, (1.0, 0.0, 0.0), (1.00, 1.00, 1.00, 1.00)),
    ("Preset_PartlyCloudy", 0.35, 0.50, 0.25, 0.45, 0.45, (1.0, 0.0, 0.0), (1.00, 1.00, 1.00, 1.00)),
    ("Preset_Cumulus",      0.55, 0.85, 0.20, 0.55, 0.65, (0.7, 0.3, 0.0), (1.00, 0.98, 0.96, 1.00)),
    ("Preset_Overcast",     0.85, 0.40, 0.35, 0.65, 0.30, (1.0, 0.0, 0.0), (0.78, 0.80, 0.85, 1.00)),
    ("Preset_Storm",        0.95, 0.30, 0.85, 0.85, 0.80, (1.0, 0.0, 0.0), (0.42, 0.45, 0.52, 1.00)),
    ("Preset_Alien",        0.65, 0.95, 0.40, 0.55, 0.90, (0.5, 0.5, 0.0), (0.55, 0.20, 0.85, 1.00)),
]

OUTPUT_DIR = Path(unreal.Paths.project_saved_dir()) / "Validation" / "MYI-60-cloud-presets"
OUTPUT_PATH = OUTPUT_DIR / "summary.json"


def log(msg):
    unreal.log(f"[Create-MyikaCloudPresets] {msg}")


def get_cloud_preset_class():
    cls = unreal.find_object(None, "/Script/MyikaSky.MyikaCloudPreset")
    if cls is None:
        log("FAIL: UMyikaCloudPreset class not found in /Script/MyikaSky")
    return cls


def ensure_directory(path: str) -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        if not unreal.EditorAssetLibrary.make_directory(path):
            raise RuntimeError(f"Failed to create content directory: {path}")


def create_preset(preset_class, asset_name, coverage, morphology, wind_strength,
                  extinction, detail, wind_dir, base_color_rgba):
    full_path = f"{PRESETS_FOLDER}/{asset_name}"
    existing = unreal.load_asset(full_path)
    if existing is not None:
        log(f"{asset_name}: already exists, updating fields")
        new_asset = existing
    else:
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", preset_class)
        new_asset = asset_tools.create_asset(asset_name, PRESETS_FOLDER, preset_class, factory)
        if new_asset is None:
            log(f"FAIL: create_asset returned None for {asset_name}")
            return False

    new_asset.set_editor_property("coverage", coverage)
    new_asset.set_editor_property("morphology", morphology)
    new_asset.set_editor_property("wind_strength", wind_strength)
    new_asset.set_editor_property("extinction", extinction)
    new_asset.set_editor_property("detail", detail)
    new_asset.set_editor_property("wind_direction",
                                  unreal.Vector(wind_dir[0], wind_dir[1], wind_dir[2]))
    new_asset.set_editor_property("base_color",
                                  unreal.LinearColor(*base_color_rgba))

    saved = unreal.EditorAssetLibrary.save_loaded_asset(new_asset, only_if_is_dirty=False)
    log(f"{asset_name}: saved={saved}")
    return saved


def write_summary(results):
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    all_ok = all(ok for (_, ok) in results)
    data = {
        "status": "ok" if all_ok else "fail",
        "presets_folder": PRESETS_FOLDER,
        "results": [{"name": name, "saved": ok} for (name, ok) in results],
        "preset_class": "/Script/MyikaSky.MyikaCloudPreset",
        "preset_count": len(PRESETS),
        "saved_count": sum(1 for (_, ok) in results if ok),
    }
    OUTPUT_PATH.write_text(json.dumps(data, indent=2), encoding="utf-8")
    log(f"summary at {OUTPUT_PATH}: {data['status']} {data['saved_count']}/{data['preset_count']}")


def main():
    log("=== START ===")
    preset_class = get_cloud_preset_class()
    if preset_class is None:
        write_summary([(spec[0], False) for spec in PRESETS])
        return

    ensure_directory(PRESETS_FOLDER)

    results = []
    for spec in PRESETS:
        try:
            ok = create_preset(preset_class, *spec)
        except Exception as e:
            log(f"EXCEPTION on {spec[0]}: {e}\n{traceback.format_exc()}")
            ok = False
        results.append((spec[0], ok))

    write_summary(results)
    log(f"=== DONE {sum(1 for (_, ok) in results if ok)}/{len(results)} saved ===")


if __name__ == "__main__":
    main()
