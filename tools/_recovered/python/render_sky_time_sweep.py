"""
Render L_SkyTest at 4 different TimeOfDay values to visually validate
V1-S2 atmosphere stack + linear sun rotation.

For each (hour, label):
  1. Set AMyikaSkyActor.PreviewTimeOfDay
  2. Call RefreshSky() to push the change through OnConstruction
  3. Take a viewport HighResShot
  4. Move/rename PNG to docs/runs/proofs/done/sky-<label>.png

Run via:
    UnrealEditor-Cmd.exe <project> /Myika/MyikaSky/L_SkyTest -unattended
        -ExecCmds="py <this-file>" -ResX=1920 -ResY=1080 -windowed -log
"""

import os
import shutil
import time
import unreal


LEVEL_PATH = "/Myika/MyikaSky/L_SkyTest"
PROOFS_DIR = r"C:\Users\jgrif\Documents\myikaai_plugin\myikai_plugin\docs\runs\proofs\done"
SCREENSHOTS_DIR = r"C:\Users\jgrif\Documents\myikaai_plugin\myikai_plugin\Saved\Screenshots\WindowsEditor"

TIME_SWEEP = [
    (6.0, "dawn"),
    (12.0, "noon"),
    (18.0, "dusk"),
    (23.0, "night"),
]


def log(msg):
    unreal.log(f"[render_sky_time_sweep] {msg}")


def find_sky_actor():
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = eas.get_all_level_actors()
    for actor in actors:
        cls = actor.get_class().get_name()
        if cls == "MyikaSkyActor" or cls.endswith("MyikaSky_C"):
            return actor
    # Fallback: any actor whose class contains "Sky" and "Myika"
    for actor in actors:
        cls = actor.get_class().get_name()
        if "Myika" in cls and "Sky" in cls:
            return actor
    return None


def latest_screenshot():
    if not os.path.isdir(SCREENSHOTS_DIR):
        return None
    files = [
        os.path.join(SCREENSHOTS_DIR, f)
        for f in os.listdir(SCREENSHOTS_DIR)
        if f.lower().endswith(".png")
    ]
    if not files:
        return None
    return max(files, key=os.path.getmtime)


def take_shot_and_move(label):
    before = latest_screenshot()
    unreal.AutomationLibrary.take_high_res_screenshot(1920, 1080, f"sky_{label}.png")
    # Wait briefly for the file to land.
    deadline = time.time() + 30
    while time.time() < deadline:
        latest = latest_screenshot()
        if latest and latest != before:
            break
        time.sleep(0.5)
    latest = latest_screenshot()
    if not latest:
        log(f"FAIL: no screenshot file appeared for {label}")
        return False
    dest = os.path.join(PROOFS_DIR, f"2026-05-03-l-sky-{label}.png")
    try:
        shutil.move(latest, dest)
        log(f"saved {dest}")
        return True
    except Exception as e:
        log(f"FAIL move {latest} -> {dest}: {e}")
        return False


def main():
    log("=== START ===")
    log(f"opening level {LEVEL_PATH}")
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.load_level(LEVEL_PATH)

    sky_actor = find_sky_actor()
    if sky_actor is None:
        log("FAIL: no AMyikaSkyActor found in level")
        return
    log(f"found sky actor: {sky_actor.get_name()}")

    if not os.path.isdir(PROOFS_DIR):
        os.makedirs(PROOFS_DIR, exist_ok=True)

    results = []
    for (hour, label) in TIME_SWEEP:
        log(f"--- {label} (TimeOfDay={hour}) ---")
        try:
            sky_actor.set_editor_property("preview_time_of_day", hour)
        except Exception as e:
            log(f"WARN set preview_time_of_day: {e}")
        try:
            sky_actor.call_method("RefreshSky")
        except Exception as e:
            log(f"WARN call RefreshSky: {e}")
        # Give the rendering pipeline a couple ticks before grabbing the shot.
        time.sleep(2.0)
        ok = take_shot_and_move(label)
        results.append((label, ok))

    log(f"=== DONE results={results} ===")


main()

try:
    unreal.SystemLibrary.quit_editor()
except Exception:
    import sys
    sys.exit(0)
