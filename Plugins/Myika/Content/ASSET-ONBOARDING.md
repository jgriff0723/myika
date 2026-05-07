# Myika Plugin — Asset Onboarding SOP

How to bring new assets into the Myika plugin so that everything stays clean, license-compliant, and shippable.

**Audience:** Jacob (sourcing assets) + Neco/Codex agents (wiring assets into systems).

**Companion docs:** ADR-0001 (operating model), ADR-0002 (MIT license), ADR-0004 (BP slider discipline).

---

## TL;DR

1. Drop new raw assets into `Plugins/Myika/Content/<system>/Imported/` — never directly into the canonical folder.
2. Add a row to `Plugins/Myika/Content/<system>/ASSETS.md` with source + license + intended use.
3. Confirm license permits redistribution (MIT-compatible, CC0, CC-BY, public domain, Quixel-Megascans-for-UE-projects, UE Engine content).
4. Neco re-imports / renames per Myika naming conventions, places in canonical folder, deletes the `Imported/` original after wiring is verified.
5. PIE proof in the relevant test map.

---

## Folder structure inside `Plugins/Myika/Content/`

```
Plugins/Myika/Content/
├── MyikaCore/
│   ├── MPC_Myika.uasset                    <- canonical
│   ├── MF_MyikaWetness.uasset
│   ├── MF_MyikaSnow.uasset
│   ├── ASSETS.md                           <- per-folder asset registry
│   └── Imported/                           <- temporary raw imports (deleted after wiring)
├── MyikaSky/
│   ├── BP_MyikaSky.uasset
│   ├── Materials/
│   │   ├── M_MyikaCloud.uasset
│   │   └── M_MyikaStars.uasset
│   ├── Presets/
│   │   └── Clouds/PCP_*.uasset
│   ├── Textures/                           <- LFS-tracked binary art
│   ├── Audio/SC_Thunder.uasset
│   ├── Debug/EUW_SkyDebug.uasset
│   ├── ASSETS.md
│   └── Imported/                           <- temporary
├── MyikaWater/
│   └── ...
└── ASSET-ONBOARDING.md                     <- this file
```

---

## Naming conventions (mandatory)

Every Myika asset follows UE community standard prefixes + Myika namespace:

| Prefix | Type | Example |
|--------|------|---------|
| `BP_Myika` | Blueprint Actor | `BP_MyikaSky`, `BP_MyikaBoat` |
| `BPC_Myika` | Blueprint Component | `BPC_MyikaWind` |
| `M_Myika` | Material | `M_MyikaCloud`, `M_MyikaWater` |
| `MI_Myika` | Material Instance | `MI_MyikaWater_Ocean` |
| `MF_Myika` | Material Function | `MF_MyikaWetness`, `MF_MyikaTriplanar` |
| `MPC_Myika` | Material Parameter Collection | `MPC_Myika` (the global one) |
| `T_Myika` | Texture 2D | `T_Myika_CloudNoise`, `T_MyikaCaustic_Flipbook` |
| `TC_Myika` | Texture Cube | `TC_MyikaStars` |
| `RT_Myika` | Render Target | `RT_MyikaShorelineRipple` |
| `SM_Myika` | Static Mesh | `SM_MyikaBoatHull`, `SM_MyikaLightningBolt` |
| `SK_Myika` | Skeletal Mesh | `SK_MyikaCharacter` |
| `AB_Myika` | Anim Blueprint | `AB_MyikaSwim` |
| `AM_Myika` | Anim Montage | `AM_MyikaInteract` |
| `AN_Myika` | Anim Notify | `AN_Myika_Footplant` |
| `SC_Myika` | Sound Cue | `SC_Myika_Thunder` |
| `SW_Myika` | Sound Wave | `SW_Myika_Footstep_Snow` |
| `MX_Myika` | Sound Mix | `MX_MyikaUnderwater` |
| `NS_Myika` | Niagara System | `NS_MyikaShorelineRipple`, `NS_MyikaRain` |
| `NE_Myika` | Niagara Emitter | `NE_MyikaRainDrop` |
| `NM_Myika` | Niagara Module | `NM_MyikaWindForce` |
| `PCG_Myika` | PCG Graph | `PCG_MyikaForest` |
| `EUW_Myika` (or just `EUW_*`) | Editor Utility Widget | `EUW_SkyDebug`, `EUW_WaterDebug` |
| `DT_Myika` | Data Table | `DT_MyikaItems`, `DT_MyikaFootstepSounds` |
| `DA_Myika` | Data Asset (general) | `DA_MyikaWeatherAttributes` |
| `PCP_Myika` (or just `PCP_*`) | PrimaryDataAsset preset | `PCP_Cumulus`, `PCP_Storm` |
| `BTT_Myika` / `BTS_Myika` | Behavior Tree task / service | `BTT_MyikaPatrol` |
| `ST_Myika` | State Tree | `ST_MyikaCombat`, `ST_MyikaRoutine` |
| `IMC_Myika` | Input Mapping Context | `IMC_MyikaSwim` |
| `IA_Myika` | Input Action | `IA_MyikaInteract` |
| `BPI_Myika` | Blueprint Interface | `BPI_MyikaInteract` |

Anything imported from outside that doesn't follow this naming gets renamed before wiring.

---

## License whitelist

Assets we can ship in the public OSS plugin:

| License | OK to ship? | Notes |
|---------|-------------|-------|
| **CC0 / Public Domain** | ✅ Yes | Best — attribution optional |
| **MIT / Apache 2.0 / BSD** | ✅ Yes | Permissive code/asset licenses |
| **CC-BY** (any version) | ✅ Yes | Attribution required — add to `ASSETS.md` |
| **CC-BY-SA** | ⚠️ Caution | Share-alike; whole derivative work must use compatible license. MIT may be incompatible. **Verify case-by-case.** |
| **CC-BY-NC** (non-commercial) | ❌ No | We're OSS but commercial use is allowed downstream. Reject. |
| **CC-BY-ND** (no derivatives) | ❌ No | We modify/integrate. Reject. |
| **UE Engine content** (Epic templates, Niagara Content Examples, Lyra, City Sample, Citizens Sample, Valley of the Ancient, Electric Dreams) | ✅ Yes | UE EULA permits redistribution as part of an Unreal project |
| **Megascans / Quixel** (free for UE) | ⚠️ **NO redistribution** | Licensed for USE in UE projects, NOT for redistribution in source. **Use locally only; do NOT push to repo.** |
| **Fab paid assets** | ❌ No | Proprietary. Use locally; do NOT push to repo. |
| **Fab free assets** | ⚠️ Verify per-asset | Many forbid redistribution. Read each license. Default = NO. |
| **NASA / USGS / ESA / public-government datasets** | ✅ Yes | Public domain in most jurisdictions |
| **Freesound.org CC0 filter** | ✅ Yes | Filter explicitly for CC0 |
| **BBC Sound Effects archive** | ⚠️ Verify | Licensed for personal/educational use; check redistribution clause |
| **Jacob's own work** | ✅ Yes | You own it; declare in ASSETS.md as "© Jacob Griffith, MIT (this repo)" |

If unsure: don't ship. Use locally, exclude from git via `.gitignore`. Already-quarantined paths (per ADR-0003): `Content/Materials/`, `Content/Characters/`, `Content/Landscape/`, `Content/Water/`, `Content/Terrain/`, `Plugins/GaeaUnrealTools/`.

---

## Workflow when Jacob drops new assets

### Step 1 — Drop into `Imported/`

Jacob creates the system's `Imported/` folder if it doesn't exist:

```
Plugins/Myika/Content/<system>/Imported/<source-pack-name>/
  cloud_noise_3d.png
  thunder_clap_01.wav
  ...
```

### Step 2 — Add to `ASSETS.md`

Edit `Plugins/Myika/Content/<system>/ASSETS.md` (create if missing). Append:

```markdown
## <pack name> — added YYYY-MM-DD

**Source:** [URL or "Jacob's own"]
**License:** CC0 / CC-BY / MIT / public domain / etc.
**Attribution required:** No / "Credit: <name>"
**Intended use:** Cloud noise texture for M_MyikaCloud
**Files:**
- `Imported/<pack>/cloud_noise_3d.png` → will become `Textures/T_MyikaCloud_Noise.png`
- `Imported/<pack>/thunder_clap_01.wav` → will become `Audio/SW_Myika_Thunder_01.wav`
```

### Step 3 — Open a Linear issue or queue task

Either:
- **If part of an existing slice** (e.g. V1-S5a needs cloud noise) — comment on the Linear issue MYI-XX with a pointer to the new ASSETS.md row. Neco picks it up next time it claims that task.
- **If standalone asset wiring** — file a queue task `2026-MM-DD-NNN-asset-wire-<pack>.md` with target_agent: nceo. The task says: import per ASSETS.md row, rename to canonical, place in canonical folder, verify in PIE, delete `Imported/<pack>/`.

### Step 4 — Neco wires it up

Neco's responsibilities:
1. Import the raw file into UE (sets up texture compression, audio settings, etc. correctly)
2. Rename per the convention table above
3. Move from `Imported/` to canonical folder (`Textures/`, `Audio/`, `Materials/`, `Meshes/`)
4. Wire into the relevant material/BP/Niagara system
5. Verify in the slice's PIE proof
6. Delete the `Imported/` original
7. Update `ASSETS.md`: cross out the "→ will become" line; add "✅ landed at `<final/path>`"

### Step 5 — License sanity scan before commit

Before any commit that adds new content:

```bash
# Confirm no quarantined paths sneaked in
git lfs ls-files | grep -E "^[a-f0-9]+ \* (Content/Materials/|Content/Characters/|Plugins/GaeaUnrealTools/)" && echo "QUARANTINE LEAK"

# Confirm every new content file has an ASSETS.md row in its system folder
# (manual inspection for now; can be a hook later)
```

---

## Where to source free assets (curated good lists)

Use these in priority order:

### Textures / 3D scans
1. **Megascans (free for UE projects)** — local use only; never commit. Best quality.
2. **AmbientCG** (CC0) — good PBR textures, freely redistributable. https://ambientcg.com
3. **Polyhaven** (CC0) — textures, HDRIs, models. https://polyhaven.com
4. **CC0 Textures by Lennart Demes** — https://cc0-textures.com (now AmbientCG)
5. **NASA SVS** — https://svs.gsfc.nasa.gov (Earth, sky, planets — public domain)

### Audio
1. **Freesound.org with CC0 filter** — https://freesound.org/search/?f=license:%22Creative+Commons+0%22
2. **BBC Sound Effects archive** — verify license per file
3. **Freepd.com** — public-domain music
4. **Pixabay Sound Effects** — CC0 license

### Skyboxes / cubemaps
1. **NASA Tycho Star Catalog** — https://svs.gsfc.nasa.gov/4851 (public domain)
2. **Polyhaven HDRIs** — CC0
3. **HDRI Haven** (now Polyhaven) — CC0

### 3D meshes
1. **Kenney.nl** — CC0 game-asset packs (good for boats, props, blockout)
2. **Polyhaven 3D models** — CC0
3. **Sketchfab CC0/CC-BY filter** — verify per-asset

### Caustics / utility flipbooks
1. **AmbientCG caustics** — CC0
2. Generate procedurally in Substance Designer / Blender (Jacob's own work)

### Niagara / VFX templates
1. **Niagara Content Examples** (Epic, free) — UE EULA permits redistribution as part of project
2. **Project Nature** (Epic free) — same

### Landscape heightmaps
1. **Tangrams Heightmapper** — CC0
2. **terrain.party** — CC-BY (NASA SRTM data underneath)
3. **Gaea free tier** — outputs heightmaps you generate (your IP)

### Foliage
1. **Quixel Megascans foliage** — local use only, never commit
2. **Polyhaven 3D plants** — CC0
3. **Tropical Foliage on AmbientCG** — CC0

---

## Common pitfalls to avoid

1. **Megascans temptation.** Megascans is free for UE projects but the license forbids redistribution of the raw assets. You can use them in your own work; you cannot ship them in our public OSS repo. Use them in local-only Content/ folders that are git-ignored, or rebuild equivalents from CC0 sources.
2. **Forgetting attribution for CC-BY.** CC-BY requires credit. Maintain a single `ATTRIBUTIONS.md` at repo root that aggregates all CC-BY credits across the plugin. Update it whenever you ship a CC-BY asset.
3. **Naming mismatch.** Imports often arrive as `cloud_noise.png`. Rename to `T_MyikaCloud_Noise.png` BEFORE importing into UE so the UE asset name is correct from the start (UE asset name = imported file basename, minus extension).
4. **Audio sample rate mismatches.** UE prefers 44.1 kHz mono for SFX. Resample on import (UE has built-in conversion).
5. **Texture compression wrong.** Set the right Texture Group in UE on import (`World`, `Character`, `UI`, etc.). Default is rarely optimal. Especially: normal maps must use `WorldNormalMap` group.
6. **LFS not configured for new extension.** If you start using a new binary extension (e.g. `.usd`, `.spp`, `.spsm`), add it to `.gitattributes` BEFORE first commit, otherwise it goes into git directly and bloats the repo permanently.
7. **Skipping `ASSETS.md` row.** "I'll add it later" never happens. Add the row first; it forces you to confirm the license.

---

## Per-system `ASSETS.md` template

Create one per system folder (`Plugins/Myika/Content/<system>/ASSETS.md`) using this template:

```markdown
# Myika<System> — Asset Registry

This file tracks the source, license, and current location of every non-trivial
asset shipped from this folder. Keeps the OSS repo legally clean.

## Format

For each pack/asset:
- **Source:** URL or "Jacob's own work"
- **License:** SPDX-style identifier
- **Attribution:** required text (or "none")
- **Files:** raw → canonical mapping

## Inventory

(none yet — add a section per pack)
```

---

## Open follow-ups

- ADR-0005 (future): per-system Megascans-equivalents catalog so agents know where to source CC0 alternatives without asking Jacob each time.
- A pre-commit hook that scans staged binary files for missing `ASSETS.md` rows and rejects the commit. Currently manual.
- A small Editor Utility script that scans `Plugins/Myika/Content/**/Imported/` and reminds Jacob/Neco of leftover raw imports that haven't been wired yet.
