"""Global state get/set."""

from __future__ import annotations

from typing import Any

from mcp.server.fastmcp import FastMCP

from ..ue_client import call


def register(mcp: FastMCP) -> None:
    @mcp.tool(
        name="myika_get_global_state",
        description=(
            "Return the current Myika global state: time_of_day (0..24), part_of_day, "
            "weather, sun/moon/wind vectors, wetness (0..1), snow_coverage (0..1), "
            "temperature (C), storm_intensity (0..1)."
        ),
    )
    async def myika_get_global_state() -> dict[str, Any]:
        return await call("get_global_state")

    @mcp.tool(
        name="myika_set_global_state",
        description=(
            "Patch one or more fields of the Myika global state. Send only the fields you "
            "want to change inside the `state` object; omitted fields keep their current value. "
            "Numeric ranges are validated; weather is one of: Clear, PartlyCloudy, Overcast, "
            "LightRain, HeavyRain, Thunderstorm, Snow, Blizzard, Foggy, SandStorm."
        ),
    )
    async def myika_set_global_state(state: dict[str, Any]) -> dict[str, Any]:
        return await call("set_global_state", {"state": state})
