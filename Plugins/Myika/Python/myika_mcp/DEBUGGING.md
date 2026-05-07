# Myika MCP — Debugging

## Where logs go

The server writes a logfile (stdio is reserved for MCP framing). Override the path with `MYIKA_MCP_LOGFILE`.

| OS | Default |
|---|---|
| Windows | `%USERPROFILE%\.myika_mcp.log` |
| macOS / Linux | `~/.myika_mcp.log` |

`MYIKA_MCP_DEBUG=1` raises level to DEBUG.

## Symptom → cause checklist

### Tool calls return `INTERNAL: could not connect to UE listener … after N attempt(s)`

The Python server reached its retry budget (default 3 retries × exponential backoff up to 5 s) without the editor accepting a connection. Check, in order:

1. Is the editor running?
2. Is the Myika plugin enabled? `Edit → Plugins → Myika`.
3. Did you click **Myika MCP → Start** in the level toolbar? The button shows ● when running, ○ when stopped.
4. Does the port match? `Project Settings → Plugins → Myika MCP → Port` must equal `MYIKA_MCP_PORT` (default 13379 on both sides).
5. Is something else on that port? `netstat -ano | findstr 13379` (Windows) or `lsof -i :13379` (Unix).
6. Try the raw protocol: `python scripts/smoke_tcp.py` — if this fails, the C++ side is broken; if it works, the Python side is broken.

### Tool calls return `NO_SUBSYSTEM: UMyikaSubsystem unavailable`

The TCP listener is up but no `UGameInstance` is available to host the subsystem. The editor must be in PIE or running a Game world. Press **Play** in the level editor. The next tool call should succeed.

### Tool calls return `UNKNOWN_TOOL`

A Phase 2 stub was registered behind the `bAllowPhase2Stubs` setting and you turned it off, OR you're hitting the legacy command name from a stale Python client. Restart the MCP client (Claude Desktop / Cursor) so it re-pulls the tool list.

### Tool calls return `INVALID_ARGS`

The error message names the offending field. Common pitfalls:
- `hours` must be 0..24 (decimals OK), not 0..23.
- `wetness` / `snow_coverage` / `storm_intensity` are 0..1, not 0..100.
- `weather` is a string, exactly one of: `Clear`, `PartlyCloudy`, `Overcast`, `LightRain`, `HeavyRain`, `Thunderstorm`, `Snow`, `Blizzard`, `Foggy`, `SandStorm`. Case-sensitive.
- `wind` x/y/z are cm/s (Unreal world units), not m/s.

### Server starts, then Claude/Cursor immediately drops it

Most often: the venv path in your client config is wrong, or the venv was created against a missing Python. Run `scripts/run_myika_mcp.bat` from a terminal — it should print the FastMCP banner. If it errors out, your config copy of the path will too.

### Editor freezes for ~10 seconds when stopping the server

A handler was mid-`RunOnGameThread` waiting on a `TFuture`. The 10 s game-thread marshalling timeout protects against deadlock; the editor will recover. Avoid this by clicking **Myika MCP → Stop** while no tool is in flight.

## Inspecting the wire protocol

```cmd
python scripts/smoke_tcp.py
python scripts/smoke_tcp.py set_time_of_day "{\"hours\": 17}"
python scripts/smoke_tcp.py set_weather "{\"weather\": \"Thunderstorm\"}"
python scripts/smoke_tcp.py get_global_state
```

Frames are 4-byte big-endian length + UTF-8 JSON body. Errors:

| Code | Meaning |
|---|---|
| `BAD_FRAME` | UE couldn't parse the bytes — usually a client bug |
| `UNKNOWN_TOOL` | Tool name not registered (check `bAllowPhase2Stubs`) |
| `INVALID_ARGS` | Argument schema/range failure; message names the field |
| `NO_SUBSYSTEM` | No `UGameInstance` available — start PIE or run game |
| `NOT_IMPLEMENTED` | Phase 2 stub, vertical not shipped |
| `INTERNAL` | Catch-all (timeout, exception, IO error) |

## Connecting [MCP Inspector](https://github.com/modelcontextprotocol/inspector)

```cmd
npx @modelcontextprotocol/inspector scripts/run_myika_mcp.bat
```

Inspector shows the full tool schema, argument types, and lets you invoke tools manually with form-validated inputs. Use it to confirm the server registers all expected tools before debugging client config.

## Resetting state

There is no "reset to defaults" tool — by design. To reset, call:

```
myika_set_global_state {"state": {
  "time_of_day": 12,
  "weather": "Clear",
  "wetness": 0,
  "snow_coverage": 0,
  "storm_intensity": 0,
  "temperature": 20
}}
```
