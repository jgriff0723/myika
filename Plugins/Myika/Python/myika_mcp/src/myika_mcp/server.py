"""FastMCP stdio server bootstrap. Tools self-register on import."""

from __future__ import annotations

import logging
import os
from contextlib import asynccontextmanager
from pathlib import Path
from typing import AsyncIterator

from mcp.server.fastmcp import FastMCP

from . import __version__
from .tools import atmosphere, diagnostics, phase2, state, time as time_tools, weather
from .ue_client import MyikaUEError, get_client

log = logging.getLogger("myika_mcp.server")


def _configure_logging() -> None:
    """File-based logging only.

    The stdio transport reserves stdout for MCP protocol frames; some clients
    also capture stderr inconsistently. Writing to a logfile is the safe
    default and matches the pattern used by chongdashu/unreal-mcp.
    """
    logfile = Path(os.getenv("MYIKA_MCP_LOGFILE", str(Path.home() / ".myika_mcp.log")))
    logfile.parent.mkdir(parents=True, exist_ok=True)

    root = logging.getLogger()
    root.setLevel(logging.DEBUG if os.getenv("MYIKA_MCP_DEBUG") else logging.INFO)
    root.handlers.clear()

    handler = logging.FileHandler(logfile, encoding="utf-8")
    handler.setFormatter(logging.Formatter(
        "%(asctime)s %(levelname)s %(name)s [%(filename)s:%(lineno)d]: %(message)s"
    ))
    root.addHandler(handler)


@asynccontextmanager
async def _server_lifespan(_server: FastMCP) -> AsyncIterator[dict]:
    """Connect to the UE listener at server start; disconnect on shutdown.

    Surfaces UE-side unavailability immediately rather than on the first tool
    call (where it would arrive as a confusing "connection refused" error
    inside Claude/Cursor). If the editor isn't up, the server still starts —
    we log the error and let the first tool call retry.
    """
    client = get_client()
    try:
        await client.connect()
        log.info("Connected to Myika UE listener.")
    except (OSError, MyikaUEError) as exc:
        log.warning("Could not pre-connect to UE listener (%s); will retry on first tool call.", exc)

    try:
        yield {}
    finally:
        log.info("Myika MCP shutting down — closing UE connection.")
        await client.close()


mcp = FastMCP(
    "myika",
    instructions=(
        "Myika MCP exposes the Myika Unreal Engine plugin's open-world global state — "
        "time of day, weather, wind, surface wetness/snow, temperature, storm intensity. "
        "Editor must be running with the Myika plugin enabled and the toolbar Start clicked. "
        "Use myika_ping first to confirm connectivity."
    ),
    lifespan=_server_lifespan,
)


def register_all_tools() -> None:
    diagnostics.register(mcp)
    state.register(mcp)
    time_tools.register(mcp)
    weather.register(mcp)
    atmosphere.register(mcp)
    phase2.register(mcp)


def main() -> None:
    _configure_logging()
    log.info("Myika MCP %s starting (stdio)", __version__)
    register_all_tools()
    mcp.run()


if __name__ == "__main__":
    main()
