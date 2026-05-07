# Reset and query

## Prompt: "What's the current state?"
**Expected tool:** `myika_get_global_state`
**Result:** LLM should report all fields legibly — time_of_day, weather, wetness, etc. Bonus points for converting `time_of_day: 17` to "5pm".

## Prompt: "Reset to defaults."
**Expected tool:** `myika_set_global_state` with:
```json
{
  "state": {
    "time_of_day": 12,
    "weather": "Clear",
    "wetness": 0,
    "snow_coverage": 0,
    "storm_intensity": 0,
    "temperature": 20,
    "wind_vector": { "x": 0, "y": 0, "z": 0 }
  }
}
```
There is no `myika_reset` tool by design — reset is just `set_global_state` to the documented defaults. The cursor rule and DEBUGGING.md include the canonical recipe.

## Prompt: "Is the editor connected?"
**Expected tool:** `myika_ping`
**Result:** Returns `pong: true` plus plugin/engine versions. If the listener is down, the LLM should explain what's wrong (toolbar Start not clicked, port mismatch, etc.) rather than retry.

## Prompt: "Compare current state to a sunny noon scene."
**Expected tools:**
1. `myika_get_global_state`
2. (no second call — LLM compares and explains in chat)

This tests whether the LLM understands that comparison doesn't require a second tool call.
