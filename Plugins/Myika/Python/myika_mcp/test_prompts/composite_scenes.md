# Composite scenes (multi-field via `set_global_state`)

These prompts change multiple fields at once. The cursor rule prefers `myika_set_global_state` for multi-field shifts; a sequence of single-field tools is also acceptable but slower.

## Prompt: "Build me a winter scene."
**Expected tool:** `myika_set_global_state` with:
```json
{
  "state": {
    "weather": "Snow",
    "snow_coverage": 1.0,
    "temperature": -5,
    "wind_vector": { "x": 200, "y": 0, "z": 0 }
  }
}
```
LLM judgment is acceptable on exact values; what matters is that snow + cold + some wind all land in one call.

## Prompt: "Make it 6am, foggy, and damp."
**Expected tool:** `myika_set_global_state` with:
```json
{
  "state": {
    "time_of_day": 6,
    "weather": "Foggy",
    "wetness": 0.5
  }
}
```
Or three separate single-field tools — both acceptable.

## Prompt: "Sunset on the beach with a light breeze."
**Expected tool:** `myika_set_global_state` with:
```json
{
  "state": {
    "time_of_day": 19,
    "weather": "Clear",
    "wind_vector": { "x": 100, "y": 50, "z": 0 }
  }
}
```
"Light breeze" should map to a small magnitude wind vector (~100 cm/s = 1 m/s ≈ 2 mph). Watch for the LLM emitting `100` thinking it means m/s — the cursor rule warns about this.

## Prompt: "Apocalypse mode."
**Expected:** LLM should compose something dramatic and label it. Acceptable result:
```json
{
  "state": {
    "weather": "Thunderstorm",
    "storm_intensity": 1.0,
    "time_of_day": 21,
    "wetness": 1.0,
    "wind_vector": { "x": 1500, "y": 0, "z": 0 }
  }
}
```
There's no "wrong" answer here — judge by whether the LLM made coherent thematic choices.
