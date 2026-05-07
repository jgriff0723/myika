# Time of day

## Prompt: "Set the time to 5pm."
**Expected tool:** `myika_set_time_of_day` with `hours: 17`
**Result:** `state.time_of_day == 17`, `part_of_day == "Afternoon"` (since hours < 17 → Afternoon, hours >= 17 → Dusk; verify against MyikaSubsystem `TimeOfDayToPart`).

## Prompt: "Make it midnight."
**Expected tool:** `myika_set_time_of_day` with `hours: 0` (or `24` — both are valid; `0` is canonical)
**Result:** `time_of_day == 0`, `part_of_day == "Night"`

## Prompt: "What time is it?"
**Expected tool:** `myika_get_global_state`
**Result:** Returns the full state object; the LLM should report `time_of_day` and `part_of_day` from the response.

## Prompt: "Skip ahead two hours."
**Expected tools (in order):**
1. `myika_get_global_state`  — read current `time_of_day`
2. `myika_set_time_of_day` with `hours: <current + 2>` (mod 24)

The LLM should NOT hardcode a value; it should read first, then write.

## Prompt: "Set the time to 25:00."
**Expected behavior:** LLM should either reject inline or call with `hours: 25` and surface the resulting `INVALID_ARGS: 'hours' must be 0..24` error. Either is acceptable; an inline rejection (without a tool call) is preferred per the cursor rule.
