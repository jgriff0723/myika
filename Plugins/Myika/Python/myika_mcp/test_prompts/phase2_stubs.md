# Phase 2 stubs

These tools are visible in the MCP schema but always return `NOT_IMPLEMENTED`. The point of these prompts is to verify the LLM behaves gracefully when a feature isn't shipped yet.

## Prompt: "Spawn a goblin in front of me."
**Expected behavior:**
- LLM may call `myika_spawn_npc` once.
- It should receive `NOT_IMPLEMENTED: 'spawn_npc' is a Phase 2 stub; the corresponding Myika vertical has not shipped yet`.
- The LLM should then explain to the user that the NPC vertical isn't built yet, and offer alternatives (e.g. setting weather/time, or asking the user to use Unreal's built-in actor placement).

**Failure mode to watch for:** LLM keeps retrying with different `class` names. The cursor rule explicitly says "Don't call them speculatively."

## Prompt: "Regenerate the PCG biomes around the player."
**Expected:** `myika_pcg_regen` → `NOT_IMPLEMENTED`. LLM should not fall back to running raw Python — Myika MCP has no `python_exec` surface.

## Prompt: "Strike that tree with lightning."
**Expected:** `myika_lightning_strike` → `NOT_IMPLEMENTED`. The LLM may suggest `myika_set_weather Thunderstorm` + `myika_set_storm_intensity 1.0` as a thematic substitute — that's a good answer.

## What to watch for

- **Retry storms.** If the LLM calls a stub repeatedly, the cursor rule isn't being applied. Re-check that `.cursor/rules/myika-mcp.mdc` is at the workspace root and the description matches the user's request.
- **Settings flip.** If `Project Settings → Plugins → Myika MCP → Allow Phase 2 Stubs` is unchecked, these tools won't be registered at all. The MCP client will then return `UNKNOWN_TOOL` instead of `NOT_IMPLEMENTED`. Either is correct; the difference is whether the user wanted the tools to discoverable in the schema.
