# Test prompts

Plain-language prompts that exercise the Myika MCP tool surface. Use these to:

1. **Smoke-test a new MCP client** — paste them into Claude Desktop / Cursor / Claude Code and confirm the right tool fires with the right args.
2. **Regression-check after schema changes** — when you rename a tool or change an arg, walk this list to make sure the LLM still maps natural language correctly.
3. **Show users what the system can do** — copy/paste fodder for onboarding docs.

Each `*.md` file is one scenario: the user prompt, the expected tool call(s) (in order), and the observable in-editor effect.

## Files

- [time_of_day.md](time_of_day.md) — single-field time changes (5pm, midnight, golden hour)
- [weather_changes.md](weather_changes.md) — discrete weather + storm intensity combinations
- [composite_scenes.md](composite_scenes.md) — multi-field shifts via `set_global_state`
- [reset_and_query.md](reset_and_query.md) — `get_global_state` + reset-to-defaults patterns
- [phase2_stubs.md](phase2_stubs.md) — confirm Phase 2 tools fail loudly with `NOT_IMPLEMENTED`

These are not automated. They're the human-in-the-loop layer that complements `pytest` (which covers the wire protocol) and the UE automation suite (which covers JSON parsing + dispatch).
