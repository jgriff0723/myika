"""Atmosphere & surface state tools."""

from __future__ import annotations

from typing import Any

from mcp.server.fastmcp import FastMCP

from ..ue_client import call


def register(mcp: FastMCP) -> None:
    @mcp.tool(
        name="myika_set_wind",
        description="Set the global wind vector in cm/s (Unreal world units).",
    )
    async def myika_set_wind(x: float, y: float, z: float) -> dict[str, Any]:
        return await call("set_wind", {"x": x, "y": y, "z": z})

    @mcp.tool(
        name="myika_set_wetness",
        description="Set surface wetness 0..1 (drives material macros).",
    )
    async def myika_set_wetness(value: float) -> dict[str, Any]:
        return await call("set_wetness", {"value": value})

    @mcp.tool(
        name="myika_set_snow_coverage",
        description="Set snow coverage 0..1 (drives material macros).",
    )
    async def myika_set_snow_coverage(value: float) -> dict[str, Any]:
        return await call("set_snow_coverage", {"value": value})

    @mcp.tool(
        name="myika_set_temperature",
        description="Set ambient temperature in degrees Celsius.",
    )
    async def myika_set_temperature(celsius: float) -> dict[str, Any]:
        return await call("set_temperature", {"celsius": celsius})
