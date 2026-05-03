# Myika

OSS bundle of open-world systems for Unreal Engine 5.7 — water, sky, weather, landscape, PCG biomes, footsteps, interaction, inventory, swimming, NPC behavior. Built natively from UE5.7 built-ins; ships with no marketplace dependencies.

## Status

**Pre-alpha.** Phase 0 plugin scaffold landed. Phase 1 (shared core: MPC, master material macros, demo character) and Phase 2 (verticals V1-V11) in progress. See `../ROADMAP.md`.

## Modules

| Module | Status | Purpose |
|--------|--------|---------|
| `MyikaCore` | ✅ Scaffold | Subsystem, shared types, MPC sync glue |
| `MyikaSky` | ⏳ Phase 2 V1 | Sky atmosphere + day-night + clouds + lightning |
| `MyikaWater` | ⏳ Phase 2 V2 | Ocean / river / lake + swimming + buoyancy |
| `MyikaLandscape` | ⏳ Phase 2 V3 | Auto-material + RVT + Nanite displacement |
| `MyikaPCG` | ⏳ Phase 2 V4 | Biome scattering + geometry decals |
| `MyikaWeather` | ⏳ Phase 2 V5 | State machine + Niagara precipitation + accumulation |
| `MyikaInteraction` | ⏳ Phase 2 V6 | Interact channel + footstep system |
| `MyikaSwap` | ⏳ Phase 2 V7 | ISM↔Actor mesh-to-actor swap |
| `MyikaItems` | ⏳ Phase 2 V8 | Inventory + crafting + vendor + pickup |
| `MyikaSwim` | ⏳ Phase 2 V9 | Swimming + diving + breath |
| `MyikaNPC` | ⏳ Phase 2 V10 | Routine + perception + combat + population |

## Install (test harness)

This plugin is developed inside the `myikai_plugin` UE5.7 project. To use it elsewhere:

1. Copy `Plugins/Myika/` into your UE5.7 project's `Plugins/` folder.
2. Regenerate project files.
3. Open the `.uproject`; UE prompts to compile new module(s).
4. Once compiled, `UMyikaSubsystem` is available as a Game Instance subsystem and `MPC_Myika` (created in Phase 1) is available to material samplers.

## License

MIT — see `../LICENSE` (added in Phase 0 cleanup).

## Contributing

Per-vertical work happens in dedicated git worktrees. See `../ROADMAP.md` "Multi-agent deployment plan" section.
