# Myika

OSS bundle of open-world systems for Unreal Engine 5.7 — water, sky, weather, landscape, PCG biomes, footsteps, interaction, inventory, swimming, NPC behavior. Built natively from UE5.7 built-ins; ships with no marketplace dependencies.

## Status

**Pre-alpha.** Phase 0 plugin scaffold landed. Phase 1 (shared core: MPC, master material macros, demo character) and Phase 2 (verticals V1-V11) in progress. See `../ROADMAP.md`.

## Modules

| Module | Status | Purpose |
|--------|--------|---------|
| `MyikaCore` | ✅ Scaffold | Subsystem, shared types, MPC sync glue |
| `MyikaMCP` | ✅ Scaffold | Editor-only MCP server bridge — exposes `UMyikaSubsystem` over loopback TCP for AI agents |
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

## MCP server (AI agent bridge)

Myika ships an optional editor-only MCP server (`MyikaMCP` module + `Python/myika_mcp/` bridge) that lets Claude Desktop / Cursor / Claude Code drive Myika global state — time of day, weather, wind, surface wetness, snow, temperature, storm intensity — via natural language.

- **Architecture.** Editor-only C++ TCP listener bound to `127.0.0.1:13379`, length-prefixed JSON framing. External Python MCP server (FastMCP, stdio) under `Python/myika_mcp/` translates MCP tool calls to UE commands.
- **Scope.** Domain-aligned to Myika. For generic editor authoring (Blueprints, asset import, level building), use [kvick-games/UnrealMCP](https://github.com/kvick-games/UnrealMCP), [chongdashu/unreal-mcp](https://github.com/chongdashu/unreal-mcp), [flopperam/unreal-engine-mcp](https://github.com/flopperam/unreal-engine-mcp), or [ArtisanGameworks/SpecialAgentPlugin](https://github.com/ArtisanGameworks/SpecialAgentPlugin) — Myika MCP coexists with any of them (tool names are prefixed `myika_*`).
- **Usage.** In the editor, click the **Myika MCP** toolbar button → **Start**. Configure your MCP client per `Python/myika_mcp/README.md`.
- **Settings.** Project Settings → Plugins → Myika MCP — port, autostart, log level, Phase 2 stub visibility.
- **Security.** Localhost-only bind, no auth. Treat editor access as the trust boundary.

## In-editor chat (`MyikaMCPChat`)

A dockable chat panel that lets you drive Myika directly from inside Unreal — no separate MCP client required. Two backends, switchable in Project Settings → Plugins → Myika MCP Chat:

- **Claude Code (local CLI)** — spawns the local `claude` binary (`--print --output-format stream-json`) and parses NDJSON to delegate events. Uses your existing Claude Code login; no API key needed. The CLI must be on `PATH` (or set `ClaudeCodeExePath` in settings).
- **Anthropic API (direct)** — posts to `api.anthropic.com/v1/messages` and runs the tool-use loop **in-process**, dispatching tool calls via `FMyikaMCPCommandRegistry::Get().Dispatch()`. Faster, requires `ANTHROPIC_API_KEY` (env var or settings field). Default model: `claude-sonnet-4-6`.

Open the panel from the level toolbar's **Myika Chat** button (also: **New Chat** to wipe history). System prompt lives at `Resources/MyikaSystemPrompt.txt` and can be overridden per project in settings. Design notes in [docs/design/myika-chat-mvp.md](docs/design/myika-chat-mvp.md).

**Deferred:** SSE streaming on the Anthropic backend, per-session JSON persistence in `Saved/MyikaMCPChat/sessions/`, retry/backoff on 429/5xx.

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
