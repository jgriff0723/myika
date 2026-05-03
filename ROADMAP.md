# myikai_plugin — Build Roadmap

**Goal:** Ship a single UE5.7 plugin that bundles all the world-building systems documented in `../myika_plugin/_chunk_*.md` (water, sky, weather, landscape, PCG, foliage, footsteps, inventory, NPCs, etc.) — built natively from UE5.7 built-ins + free assets, generated through AI agents working in parallel worktrees.

**Strategy:** Don't reinvent. Each system maps 1:1 to a chunk's "How WE can build this in UE5.7 (free path)" recipe. AI agents execute those recipes; the plugin is the accumulated output.

**Approach:** Vertical slices. One subsystem = one worktree = one agent (or pair). Shared core code is serialized (one agent at a time). Demo map (`Haven.umap`) is the integration test bench.

## Locked operating model (grilled 2026-05-03)

| # | Decision | Implication |
|---|----------|-------------|
| **G1** | Code lives at `Plugins/Myika/Source/{MyikaCore, MyikaSky, MyikaWater, ...}` and content at `Plugins/Myika/Content/` | Plugin is portable; the `myikai_plugin` UE project is just a test harness. Combat/Platforming/SideScrolling variants stay in the project, NOT in the plugin |
| **G2** | Hands-off mode: **end-of-vertical review only**. Failures escalate immediately; successes stack silently | Agents finish vertical → PIE proof → screen recording → Linear issue closure. Jacob only sees vertical-complete pings or escalations |
| **G3** | Rolling **2-3 verticals in flight** after Phase 1 | Manageable cognitive load if anything escalates; merges land smoothly to main; ~5-6 waves to clear V1-V11 |
| **G4** | Tiered escalation budgets: **3 build attempts / 5 PIE attempts / instant on aesthetic / 2hr wall-clock cap** | Agent self-bounds. Aesthetic judgments always escalate (only humans judge looks). Hard timeout prevents runaway compute |
| **G5** | **Linear `Unreal_myika`** = strategic source of truth (1 issue per vertical, attachable PIE recordings). **Workshop queue** = tactical execution locks (prevents two agents touching `MPC_Myika` simultaneously) | Two layers, different jobs. Nothing conflicts |

**Current state baseline (verified 2026-05-03):**
- UE 5.7 project with **40+ relevant plugins already enabled** (Water/Niagara/PCG/Geometry/StateTree/SunPosition/Volumetrics/etc.)
- Source module: `Source/myikai_plugin/` (Combat/Platforming/SideScrolling Epic template variants)
- Content scaffolds present: `Content/Water/{Beach,EndlessOcean,OceanWater,RenderTargets}`, `Content/Terrain/{AssetDepot,Haven,PCGBiome}.umap`, `Content/Landscape/{Materials,Textures}`
- Third-party: `Plugins/GaeaUnrealTools` (heightmap → terrain workflow)
- Codex docs: `docs/agents/`, `CONTEXT.md`, `AGENTS.md`
- Linear project: `Unreal_myika` (Myika AI team)

---

## Phase 0 — Foundation (week 1, mostly done; gaps to close)

| Step | Status | Owner | Notes |
|------|--------|-------|-------|
| 0.1 UE5.7 project + plugin set enabled | ✅ Done | — | Verified in `myikai_plugin.uproject` |
| 0.2 Shared core module decision | ⚠️ Open | Jacob | Build as **project module** (current `Source/myikai_plugin/`) OR break out to `Plugins/Myika/Source/Myika.uplugin` for portability? **Recommend: Plugin from the start** so it can ship to other projects. Move the variant content out (Combat/Platforming/SideScrolling are demos, not plugin core). |
| 0.3 `MPC_Myika` Material Parameter Collection | ❌ TODO | Neco | Single global MPC: `TimeOfDay`, `SunDirection`, `MoonDirection`, `WindVector`, `Wetness`, `SnowCoverage`, `Temperature`, `LightningFlash`, `StormIntensity`, `WaterLevel`. Source-of-truth for every shader. |
| 0.4 Master material macros | ❌ TODO | Neco | `MF_MyikaWetness`, `MF_MyikaSnow`, `MF_MyikaWind`, `MF_MyikaTriplanar` — drop-in macros every material in the plugin uses. |
| 0.5 Worktree + multi-agent workflow set up | ⚠️ Partial | Jacob | Use `superpowers:using-git-worktrees` skill. One worktree per Phase-2 vertical. Agents commit to feature branches; serialized merges to `main`. |
| 0.6 Linear backlog populated | ⚠️ Partial | Teri | Use `to-issues` skill on each Phase-2 vertical below to generate workshop queue tasks + Linear issues. |
| 0.7 Demo map = `Haven.umap` | ⚠️ Partial | — | Already exists. Becomes the **integration test bench** — every vertical adds itself to Haven and the result is the proof. |
| 0.8 `myika_test.txt` cleanup + plugin module scaffold | ❌ TODO | Neco | Stub `Plugins/Myika/Source/Myika/Myika.Build.cs` + `Myika.uplugin` so Phase 2 verticals have a place to land code. |

**Gate to Phase 1:** 0.2-0.4, 0.8 done. Worktree workflow rehearsed (0.5).

---

## Phase 1 — Shared content foundation (week 1-2, 1 agent)

These are NOT verticals — they're things every vertical depends on, so build first, build once. Single agent (Neco) sequentially to avoid merge conflicts.

| Step | Source chunk | Deliverable |
|------|--------------|-------------|
| 1.1 `MPC_Myika` (above 0.3) — bind to all globals | All chunks | One `.uasset` |
| 1.2 Master material macros (above 0.4) | 01, 02, 03, 13 | 4-6 `.uasset` MF |
| 1.3 `BP_MyikaSettings` actor — runtime singleton with MPC writes, exposed BP API | All chunks | One BP class |
| 1.4 `MyikaSubsystem` (UGameInstanceSubsystem) — global event bus + service locator | All chunks | One C++ class |
| 1.5 `BP_MyikaDemoCharacter` — extend ThirdPersonCharacter with hooks for swim, footsteps, weather reactions | 05, 07, 06 | One BP class |
| 1.6 Test map cleanup — `Haven.umap` becomes the canonical showcase, `Untitled.umap` deleted | — | Cleaner content tree |

**Gate to Phase 2:** All 6 land. PIE on Haven shows MPC values being driven by a debug widget.

---

## Phase 2 — Verticals in parallel (weeks 2-N, multiple agents in worktrees)

Each vertical = one worktree, one agent (Claude Code or Codex), built from the corresponding chunk's "How WE can build this in UE5.7" recipe. **Parallelizable** because each lives in its own asset folder + module + test map. Merge to `main` serially after each vertical's PIE proof.

Recommended build order (visual impact + dependency order):

### V1 — Sky / Atmosphere / Day-Night (chunks 02, 15)
- Chunks: `_chunk_02_ultradynamicsky.md`, `_chunk_15_sky_creator.md`
- Module: `Plugins/Myika/Source/MyikaSky/`
- Folder: `Content/Myika/Sky/`
- Deliverables:
  - `BP_MyikaSky` — owns SkyAtmosphere + Volumetric Cloud + DirectionalLight + SkyLight + ExpHeightFog
  - Curve atlases for sun color/intensity, sky luminance, fog density, exposure
  - 6 cloud presets (clear, cumulus, partly cloudy, overcast, storm, alien)
  - Stars/moon sub-actor with cubemap
  - SunPosition plugin integration (real-world solar)
  - Lightning timeline
  - MPC writes
- Test map: `Content/Myika/Sky/L_SkyTest.umap`
- PIE proof: 24-hour cycle plays correctly; cloud preset switching works.

### V2 — Water (chunks 01, 09 Oceanology/Riverology)
- Chunks: `_chunk_01_imaginaryblend.md` (Fluid Flux), `_chunk_09_fab_listings.md` (Oceanology, Riverology)
- Module: `Plugins/Myika/Source/MyikaWater/`
- Folder: `Content/Myika/Water/` (extend existing scaffold)
- Deliverables:
  - `BP_MyikaOcean` — UE Water `WaterBodyOcean` + custom Single Layer Water material with Lumen
  - `BP_MyikaRiver` — `WaterBodyRiver` with flow texture + obstacle-aware foam
  - `BP_MyikaLake` — `WaterBodyLake`
  - Niagara Fluids 2D shallow-water shoreline ripple solver
  - Underwater post-process material (depth fade, fog, refraction, waterline)
  - Caustic decal flipbook
  - `BP_MyikaBuoyancy` — wraps UE Water BuoyancyComponent with sane defaults
- Test map: `Content/Myika/Water/L_WaterTest.umap`
- PIE proof: ocean Gerstner, river flow, character swims, boat floats.

### V3 — Landscape Auto-Material + RVT (chunks 03, 13)
- Chunks: `_chunk_03_brushify.md`, `_chunk_13_panoramake.md`
- Module: `Plugins/Myika/Source/MyikaLandscape/`
- Folder: `Content/Myika/Landscape/` (extend existing)
- Deliverables:
  - `M_MyikaLandscape` — auto-material with 8 layers (rock/grass/moss/sand/dirt/snow/leaves/mud)
  - Triplanar cliff projection by slope
  - Height-based snow line
  - Curvature mud
  - World-position macro variation
  - Splatmap input (4-channel RGBA from Gaea/World Machine)
  - **RVT output** for landscape→prop blending
  - `MF_MyikaLandscapeAtlas` — distance mesh atlas material
  - 12 sculpt brush heightmaps (free downloads + GAEA exports)
- Test map: extend `Haven.umap`
- PIE proof: paint+auto layers blend correctly; Nanite displacement on UE5.4+.

### V4 — PCG Biome (chunks 04, 10)
- Chunks: `_chunk_04_scanslibrary_pwg.md`, `_chunk_10_munduscreatus_pcg.md`
- Module: `Plugins/Myika/Source/MyikaPCG/`
- Folder: `Content/Myika/PCG/`
- Deliverables:
  - `PCG_MyikaForest` — layered PCG graph (canopy → understory → grass → rocks → debris)
  - 3 biome presets (PineForest, TropicalIsland, Arid) — swap asset pools
  - Density propagation subgraph (3 density bands)
  - Water/Tag exclusion subgraph
  - `BP_MyikaGeometryDecal` — terrain-conforming geometry decal stamper (Munduscreatus pattern) with bake-to-static-mesh
- Test map: extend `Content/Terrain/PCGBiome.umap`
- PIE proof: drop a PCG volume on Haven, see forest scatter correctly with biome switch.

### V5 — Weather + Precipitation (chunks 02, 09 Easy Rain, 21 Easy Snow / Ultra Volumetrics)
- Chunks: `_chunk_02_ultradynamicsky.md`, `_chunk_09_fab_listings.md` (Easy Rain), `_chunk_21_googledrive_assets.md` (Easy Snow / Ultra Volumetrics)
- Module: `Plugins/Myika/Source/MyikaWeather/` (could merge with MyikaSky)
- Folder: `Content/Myika/Weather/`
- Deliverables:
  - `BP_MyikaWeather` — biome-volume-driven weather state machine (Markov transitions)
  - Niagara emitters: rain, snow (mesh + sprite), hail, sand, ash, dust, leaves
  - Player-following SceneCaptureComponent2D occlusion RT (no rain under roofs)
  - Wetness/snow accumulation RT (player-following, persistent for trails)
  - Post-process material with screen rain droplets, frost, heat haze
  - Lightning timeline (cloud flash + bolt mesh + thunder)
- Test map: extend Haven.umap
- PIE proof: weather state transitions; snow accumulates in player wake; rain doesn't penetrate roofs.

### V6 — Interaction + Footsteps (chunk 05, 07)
- Chunks: `_chunk_05_hyper_environment.md` (footstep), `_chunk_07_hyper_systems.md` (interaction)
- Module: `Plugins/Myika/Source/MyikaInteraction/`
- Folder: `Content/Myika/Interaction/`
- Deliverables:
  - `BPI_MyikaInteract` interface
  - `AC_MyikaInteractionComponent` — line trace + prompt UI
  - Custom Interaction Channel
  - Anim notify `AN_Myika_Footplant` + `BP_MyikaFootstepComponent`
  - Physical surface registry (Snow, Sand, Mud, Rock, Wood, Metal, Water)
  - DT_FootstepSounds / DT_FootstepFX / DT_FootstepDecals data tables
- Test map: extend Haven; characters trigger surface-aware footsteps; objects show interact prompts
- PIE proof: walk across surfaces, hear right sounds; press E on object, interact prompt + action fires.

### V7 — Mesh-to-Actor Swap (chunk 05)
- Chunk: `_chunk_05_hyper_environment.md` (Mesh-to-Actor Swap)
- Module: `Plugins/Myika/Source/MyikaSwap/`
- Folder: `Content/Myika/Swap/`
- Deliverables:
  - Custom collision channel `MTA_Swap`
  - `AC_MyikaSwapper` (player) + `AC_MyikaRespawnActor` (actor) + `BP_MyikaSwapManager` (singleton)
  - DT_SwappableMeshes
  - PCG / foliage / direct-placement collision setup utilities
- Test map: 100k+ ISMs on Haven, walk near them, see actor swap + respawn after timer.
- PIE proof: 70+ FPS on dense scene; swap works on ISM/HISM/Direct; multiplayer state replicates.

### V8 — Inventory + Crafting + Vendor + Pickup (chunk 06)
- Chunk: `_chunk_06_hyper_items.md`
- Module: `Plugins/Myika/Source/MyikaItems/`
- Folder: `Content/Myika/Items/`
- Deliverables:
  - `AC_MyikaInventory` central manager
  - `BP_MyikaInventoryContainer` actor (Default / Container / Equipment / Crafting / Hotbar types)
  - DT_Items master spec
  - SaveGame manager
  - Decay + durability components
  - `BP_MyikaCraftingStation` + `AC_MyikaVendor` + `AC_MyikaItemAllocator`
  - `BP_MyikaPickup` master + foliage swap integration (depends on V7)
  - UMG inventory panel + tooltip system
- Test map: pick up sticks/stones, open inventory, craft a torch, sell to vendor
- PIE proof: full inventory loop works; save/load preserves nested containers + decay state.

### V9 — Swimming (chunk 07)
- Chunk: `_chunk_07_hyper_systems.md` (swimming)
- Module: `Plugins/Myika/Source/MyikaSwim/` (or fold into MyikaWater)
- Folder: `Content/Myika/Swim/`
- Deliverables:
  - `BP_MyikaSwimComponent` — extends `UCharacterMovementComponent` for swim/dive
  - Water-volume detection (uses UE Water plugin's Physics Volume integration)
  - Camera-forward movement under water
  - `BP_MyikaBreathComponent` with oxygen depletion
  - Underwater post-process + ambient sound
- Test map: extend WaterTest — character enters ocean, swims, dives, oxygen depletes, surfaces.
- PIE proof: full swim/dive loop with breath + UI.

### V10 — NPC AI (chunks 07, Lyra/Citizens reference)
- Chunk: `_chunk_07_hyper_systems.md` (Routine + Perception + Combat + Population)
- Module: `Plugins/Myika/Source/MyikaNPC/`
- Folder: `Content/Myika/NPC/`
- Deliverables (scope this carefully — full Hyper-tier is 8-12 weeks):
  - `BP_MyikaPopulationManager` (bounded volume spawn/despawn, distance LOD)
  - `BP_MyikaRoutineNPC` with State Tree + Smart Object integration (Citizens Sample reference)
  - DT_RoutineProfiles (Villager, Guard, Wildlife)
  - Predefined behaviors via Gameplay Behaviors (Patrol, Roam, Sit, Sleep, Eat, Tend Shop)
  - Spline patrol smart objects
  - Custom `AC_MyikaPerception` (sight + hearing + tag-based threat events)
  - Alert Proxy pattern
  - Basic combat State Tree (melee, ranged, fallback)
- Test map: village in Haven with 5 villagers + 2 guards going about routines; aggro on player
- PIE proof: routines visible; perception + alert escalation works; combat triggers cleanly.

### V11 — Niagara UI Renderer fork (chunk 12)
- Chunk: `_chunk_12_niagara_ui.md`
- Approach: **Fork the open-source plugin into `Plugins/NiagaraUIRenderer/`** rather than reimplement.
- Action: clone `https://github.com/SourisDev/NiagaraUIRenderer`, drop into Plugins, verify build with UE 5.7.
- Time: 1 day.

---

## Phase 3 — AI integration: skill wrappers (weeks N+1, after V1-V5)

Once verticals V1-V5 land, the **prompt-callable skill wrappers** (the actual point of myika as an AI plugin) get built. Each wrapper composes a vertical's primitives into one function call.

| Skill | Composes |
|-------|----------|
| `gen_sky_atmosphere_stack(time_of_day, weather)` | V1 |
| `gen_day_night_cycle(speed)` | V1 |
| `gen_volumetric_cloud_material(preset)` | V1 |
| `gen_weather_state_machine(types, transitions)` | V5 |
| `gen_water_body(type, location, size)` | V2 |
| `gen_river_spline(spline_points)` | V2 |
| `gen_landscape_automaterial(layers, splatmap)` | V3 |
| `gen_pcg_biome(biome, region)` | V4 |
| `gen_geometry_decal_stamp(mesh, location)` | V4 |
| `gen_swap_zone(meshes, density)` | V7 |
| `gen_inventory_for_actor(items)` | V8 |
| `gen_npc_routine(profile, region)` | V10 |

These get implemented either:
- **A.** In the fresh AI tooling we build to drive `myikai_plugin` (TBD architecture — clean slate from MyikaAI/myika-unreal failure)
- **B.** As Editor Utility Blueprints in the plugin itself (works without any AI tooling — useful for non-AI users)

Recommend **B first** (Editor Utilities are valuable standalone), then **A** as a later layer.

---

## Multi-agent deployment plan

**Pattern (from workshop):**
1. **Teri** owns the Linear backlog + queue tasks; runs `to-issues` per vertical.
2. **Myika** (you) owns this roadmap + decisions about asset choices, art direction, demo map composition.
3. **Neco** owns implementation per vertical: each vertical becomes a Neco worktree.
4. **OpenClaw / parallel Claude Code sessions** can pick up sibling verticals in their own worktrees once Phase 0/1 ground rules exist.

**Worktree pattern per vertical:**
```
git worktree add ../myikai_plugin-V1-sky feature/myika-sky
# Spin Claude Code in worktree
# Agent reads ../myika_plugin/_chunk_02_ultradynamicsky.md + _chunk_15_sky_creator.md
# Agent implements per V1 deliverables
# Agent ships PIE proof + run log + Linear issue closure
# Merge to main when V1 complete
```

**Serialization rules:**
- **Phase 0/1** = serial (shared core; conflicts certain).
- **Phase 2 V1-V11** = parallel-safe within isolated module + content folder; merges serial.
- **Phase 3 skill wrappers** = serial (touches single tool registry).

**Coordination:**
- Linear `Unreal_myika` board tracks vertical progress.
- Workshop queue lock files prevent two agents touching `MPC_Myika` or `M_MyikaLandscape` shared assets at once.
- Daily memory + handoff files keep the broader context coherent.

---

## Asset creation philosophy

When an agent needs an asset:

1. **First check free sources:**
   - Megascans (free for UE) — terrain textures, plants, rocks
   - Niagara Content Examples — particle templates
   - Lyra Sample — gameplay code references
   - Citizens Sample — NPC architecture reference
   - Epic open-world demos (Valley of the Ancient, Electric Dreams, City Sample) — auto-materials, cloud presets, distance meshes
   - Project Nature — water, sky reference
   - Open-source community packs (linked in chunk "Free / open-source assets" sections)

2. **Generate procedurally if cheap:**
   - Heightmaps via Gaea (already have plugin) or noise functions
   - Distance meshes via baked landscape silhouettes (Modeling Mode)
   - Geometry decals via PCG + Geometry Script (V4 deliverable)

3. **AI-generate as last resort:**
   - Cloud/weather noise textures via Stable Diffusion → import (matches DynamicSky Phase 2 idea from MyikaAI roadmap)
   - Hero textures only when Megascans has no equivalent

4. **Never:**
   - Buy marketplace assets (defeats the OSS goal)
   - Use copyrighted material

---

## Demo map plan

**`Haven.umap`** is the integration test bench. As each vertical lands, it adds itself to Haven:

| After | Haven contains |
|-------|---------------|
| Phase 1 | Empty landscape + ThirdPersonCharacter + debug MPC widget |
| V1 | Sky cycling 24h above Haven |
| V2 | Ocean at the boundary, river through middle, lake in interior |
| V3 | Auto-material painted across landscape (cliffs, grass, snow caps) |
| V4 | Forest biome scattered via PCG; geometry decals on rocks |
| V5 | Weather state machine — Haven cycles through clear → rain → storm → snow |
| V6 | Footsteps surface-aware; a few interactable props (lever, door) |
| V7 | Forest = ISMs that swap to actors when player approaches |
| V8 | Pickup sticks/stones; inventory; craft torch; vendor in clearing |
| V9 | Swim across the lake; dive in ocean |
| V10 | Village of 5 villagers + 2 guards + wildlife wandering Haven |
| V11 | UI HUD with Niagara particle effects |

**Final state:** Haven is a complete walkable demo of every system, doubles as marketing footage and as the canonical regression test before any release.

---

## Verification per vertical

Every vertical must ship:
1. **Compile proof** — both `Editor` and `Game` targets build.
2. **PIE proof** — vertical works in PIE on its dedicated test map AND in `Haven.umap`.
3. **Asset proof** — all `.uasset` files committed and named per convention (`BP_Myika*`, `M_Myika*`, `MF_Myika*`, `DT_Myika*`, `MPC_Myika*`).
4. **Run log** — under `docs/runs/` (Codex pattern from existing AGENTS.md) or `.workshop/logs/`.
5. **Linear issue closure** — with PIE screenshot/screenrecording attached.
6. **Documentation page** — short `docs/systems/<vertical>.md` describing API + integration points + how to use the system from BP/C++.

---

## What to NOT do

- **No marketplace plugin dependencies.** Everything from UE built-ins + Megascans-tier free assets only.
- **No premature multi-agent specialist orchestration** — that was a previous over-reach. Single Claude conversations per worktree are fine for v1.
- **No AI-tooling layer until V5+** — build the actual content systems first; the AI skill wrappers (Phase 3) come after the primitives exist.
- **No re-architecture mid-vertical.** Each vertical is scoped per its chunk; no scope creep.
- **No skipping verification.** Each vertical's PIE proof gates merge to main.

---

## First concrete actions (this week)

1. **Decide 0.2** — game-module vs plugin-module separation. Recommend plugin (`Plugins/Myika/`).
2. **Spin Neco** on Phase 1 (steps 1.1-1.6) sequentially. ~2-3 days.
3. **Once Phase 1 lands**, use `to-issues` (Teri) to break V1 (Sky) and V2 (Water) into Linear issues.
4. **Open two worktrees** — one for V1 Sky, one for V2 Water. Spin a Claude Code session in each. They each read their corresponding chunks and start building.
5. **Set up daily smoke** — `workshop-smoke` on Haven.umap to catch regressions as verticals land.

---

*Roadmap drafted 2026-05-03. Living doc — update as verticals land or scope shifts.*
