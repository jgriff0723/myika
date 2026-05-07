"""Time of day."""

from __future__ import annotations

from typing import Any

from mcp.server.fastmcp import FastMCP

from ..ue_client import call


def register(mcp: FastMCP) -> None:
    @mcp.tool(
        name="myika_set_time_of_day",
        description="Set the in-game time of day. `hours` is 0..24 (e.g. 17.0 = 5pm).",
    )
    async def myika_set_time_of_day(hours: float) -> dict[str, Any]:
        return await call("set_time_of_day", {"hours": hours})
