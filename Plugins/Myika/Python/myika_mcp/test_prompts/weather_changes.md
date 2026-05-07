# Weather changes

## Prompt: "Make it rain."
**Expected tool:** `myika_set_weather` with `weather: "LightRain"` (or `"HeavyRain"` for "really pour" / "heavy rain")
**Result:** `state.weather == "LightRain"`. The LLM should NOT also set wetness — Phase 1 verticals are responsible for deriving surface state from weather.

## Prompt: "Make it stormy."
**Expected tools (in order):**
1. `myika_set_weather` with `weather: "Thunderstorm"`
2. `myika_set_storm_intensity` with `value: 0.8` (range guidance: 0.6–1.0 for "stormy"; 1.0 for "violent storm")

Either order is acceptable. A composite via `myika_set_global_state` is also acceptable.

## Prompt: "Clear it up."
**Expected tool:** `myika_set_weather` with `weather: "Clear"`. LLM may also reset `storm_intensity` to 0 if it was high — bonus points but not required.

## Prompt: "Sandstorm."
**Expected tool:** `myika_set_weather` with `weather: "SandStorm"`. Note the camelcase — common LLM mistake is `"sandstorm"` (lowercase).

## Prompt: "Make it foggy."
**Expected tool:** `myika_set_weather` with `weather: "Foggy"`.

## Prompt: "Hail of frogs."
**Expected behavior:** LLM should explain that this isn't a supported weather state and list valid options. If it tries the call, expect `INVALID_ARGS: unknown weather 'HailOfFrogs'`.
