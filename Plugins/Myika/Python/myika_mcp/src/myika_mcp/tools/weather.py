"""Weather tools."""

from __future__ import annotations

from typing import Any, Literal

from mcp.server.fastmcp import FastMCP

from ..ue_client import call

WeatherName = Literal[
    "Clear",
    "PartlyCloudy",
    "Overcast",
    "LightRain",
    "HeavyRain",
    "Thunderstorm",
    "Snow",
    "Blizzard",
    "Foggy",
    "SandStorm",
]


def register(mcp: FastMCP) -> None:
    @mcp.tool(
        name="myika_set_weather",
        description=(
            "Set the discrete weather state. One of: Clear, PartlyCloudy, Overcast, "
            "LightRain, HeavyRain, Thunderstorm, Snow, Blizzard, Foggy, SandStorm."
        ),
    )
    async def myika_set_weather(weather: WeatherName) -> dict[str, Any]:
        return await call("set_weather", {"weather": weather})

    @mcp.tool(
        name="myika_set_storm_intensity",
        description="Continuous storm intensity 0..1 used by sky/cloud morphology.",
    )
    async def myika_set_storm_intensity(value: float) -> dict[str, Any]:
        return await call("set_storm_intensity", {"value": value})
