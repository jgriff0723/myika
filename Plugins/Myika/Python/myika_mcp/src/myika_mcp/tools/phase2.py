"""Phase 2 stubs.

These tools are exposed so AI agents can discover them via the schema, but the
UE side returns NOT_IMPLEMENTED until the corresponding Myika vertical ships.
"""

from __future__ import annotations

from typing import Any

from mcp.server.fastmcp import FastMCP

from ..ue_client import call


def register(mcp: FastMCP) -> None:
    @mcp.tool(
        name="myika_pcg_regen",
        description="(Phase 2) Trigger PCG biome regeneration. Currently returns NOT_IMPLEMENTED.",
    )
    async def myika_pcg_regen(region: str | None = None) -> dict[str, Any]:
        return await call("pcg_regen", {"region": region or ""})

    @mcp.tool(
        name="myika_spawn_npc",
        description="(Phase 2) Spawn an NPC at a given location. Currently returns NOT_IMPLEMENTED.",
    )
    async def myika_spawn_npc(npc_class: str, x: float, y: float, z: float) -> dict[str, Any]:
        return await call("spawn_npc", {"class": npc_class, "x": x, "y": y, "z": z})

    @mcp.tool(
        name="myika_lightning_strike",
        description="(Phase 2) Trigger a lightning strike at a location. Currently returns NOT_IMPLEMENTED.",
    )
    async def myika_lightning_strike(x: float, y: float, z: float) -> dict[str, Any]:
        return await call("lightning_strike", {"x": x, "y": y, "z": z})
