"""Health-check tool."""

from __future__ import annotations

from typing import Any

from mcp.server.fastmcp import FastMCP

from ..ue_client import call


def register(mcp: FastMCP) -> None:
    @mcp.tool(name="myika_ping", description="Check that the Myika UE plugin is reachable. Returns plugin and engine versions.")
    async def myika_ping() -> dict[str, Any]:
        return await call("ping")
