# Myika MCP

External Python MCP server that bridges Claude Desktop / Cursor / Claude Code to the Myika Unreal plugin's `UMyikaSubsystem` via a localhost TCP listener.

## What it exposes

Domain-aligned tools — **not** a generic Unreal editor MCP. For generic editor authoring (Blueprints, asset import, level building) use one of:

- [kvick-games/UnrealMCP](https://github.com/kvick-games/UnrealMCP)
- [chongdashu/unreal-mcp](https://github.com/chongdashu/unreal-mcp)
- [flopperam/unreal-engine-mcp](https://github.com/flopperam/unreal-engine-mcp)
- [ArtisanGameworks/SpecialAgentPlugin](https://github.com/ArtisanGameworks/SpecialAgentPlugin)

Tool surface (all prefixed `myika_*` to avoid namespace collisions if you run another Unreal MCP server alongside this one):

| Tool | Args | Effect |
|---|---|---|
| `myika_ping` | — | Health check; returns plugin/engine version |
| `myika_get_global_state` | — | Return the full `FMyikaGlobalState` |
| `myika_set_global_state` | `state` (partial dict) | Patch any subset of fields |
| `myika_set_time_of_day` | `hours` (0..24) | Set time + auto-derived part-of-day |
| `myika_set_weather` | `weather` (enum string) | Set discrete weather |
| `myika_set_storm_intensity` | `value` (0..1) | Continuous storm intensity |
| `myika_set_wind` | `x, y, z` (cm/s) | Set wind vector |
| `myika_set_wetness` | `value` (0..1) | Set surface wetness |
| `myika_set_snow_coverage` | `value` (0..1) | Set snow coverage |
| `myika_set_temperature` | `celsius` | Set ambient temperature |

Phase 2 stubs visible in the schema but returning `NOT_IMPLEMENTED` until the corresponding Myika vertical ships: `myika_pcg_regen`, `myika_spawn_npc`, `myika_lightning_strike`.

## Install

Requires Python **3.10+**. Run from this directory:

```cmd
scripts\setup_myika_mcp.bat
```

The script prefers [`uv`](https://docs.astral.sh/uv/) if it's on PATH; otherwise it falls back to the `py` launcher and stdlib `venv`.

## Run

The Myika plugin must be enabled and the editor must be running with the toolbar **Myika MCP → Start** clicked. Then either:

```cmd
scripts\run_myika_mcp.bat
```

or wire it into your MCP client.

### Claude Desktop

`%APPDATA%\Claude\claude_desktop_config.json`:

```json
{
  "mcpServers": {
    "myika": {
      "command": "C:\\Users\\jgrif\\Documents\\myikaai_plugin\\myikai_plugin\\Plugins\\Myika\\Python\\myika_mcp\\scripts\\run_myika_mcp.bat"
    }
  }
}
```

### Cursor

`.cursor/mcp.json` in your project root, same shape under `mcpServers`.

### Claude Code

```cmd
claude mcp add myika -- "C:\Users\jgrif\Documents\myikaai_plugin\myikai_plugin\Plugins\Myika\Python\myika_mcp\scripts\run_myika_mcp.bat"
```

## When something breaks

See [DEBUGGING.md](DEBUGGING.md) — symptom → cause checklist, logfile location, wire-protocol error codes, and recovery recipes.

## Cursor / Claude Code rule file

A `.cursor/rules/myika-mcp.mdc` at the workspace root (`myikai_plugin/.cursor/rules/`) primes Cursor (and Claude Code via its rules support) with Myika-specific guidance: preconditions, the unit gotchas (`hours` 0..24, `wetness` 0..1, wind in cm/s), the weather enum, natural-language → tool mapping, and Phase 2 stub behavior. Loaded contextually — only when the model is working with Myika weather/time/state.

## Verify

Without an MCP client, raw TCP smoke:

```cmd
python scripts\smoke_tcp.py
python scripts\smoke_tcp.py set_time_of_day "{\"hours\": 17}"
```

With [MCP Inspector](https://github.com/modelcontextprotocol/inspector) — convenience launcher:

```cmd
scripts\mcp_inspector.bat
```

(Equivalent to `npx -y @modelcontextprotocol/inspector scripts\run_myika_mcp.bat`.)

## Test prompts

The [test_prompts/](test_prompts/) folder contains real plain-language scenarios with expected tool-call sequences — useful for smoke-testing a new MCP client config or regression-checking after schema changes.

## Environment variables

| Var | Default | Meaning |
|---|---|---|
| `MYIKA_MCP_HOST` | `127.0.0.1` | UE listener host |
| `MYIKA_MCP_PORT` | `13379` | UE listener port (must match plugin Project Settings → Plugins → Myika MCP) |
| `MYIKA_MCP_REQUEST_TIMEOUT_S` | `10` | Per-request timeout |
| `MYIKA_MCP_CONNECT_TIMEOUT_S` | `5` | Initial connect timeout |
| `MYIKA_MCP_LOGFILE` | `~/.myika_mcp.log` | Where the server writes logs (stdio is reserved for MCP) |
| `MYIKA_MCP_DEBUG` | unset | Set to any value to log at DEBUG level |

## Wire protocol (for debugging or non-Python clients)

Length-prefixed UTF-8 JSON over loopback TCP. Each frame is `[4-byte big-endian uint32 length][JSON body]`.

Request:
```json
{ "id": "uuid-or-int", "tool": "set_time_of_day", "args": { "hours": 17 } }
```

Response:
```json
{ "id": "...", "ok": true,  "result": { "state": { ... } } }
{ "id": "...", "ok": false, "error": { "code": "INVALID_ARGS", "message": "..." } }
```

Error codes: `BAD_FRAME`, `UNKNOWN_TOOL`, `INVALID_ARGS`, `NO_SUBSYSTEM`, `NOT_IMPLEMENTED`, `INTERNAL`.

## Security model

The C++ listener binds **127.0.0.1 only**. There is no auth. Anyone with local-machine access to the editor can drive Myika state — same trust model as having the editor open. Do not expose this port to other hosts.

## Tests

```cmd
scripts\setup_myika_mcp.bat
.venv\Scripts\activate
pip install -e ".[test]"
pytest
```
