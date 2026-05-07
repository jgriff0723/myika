"""Quick local sanity check: import the server, register tools, list names."""

from __future__ import annotations

import asyncio

from myika_mcp import server


async def main() -> None:
    server.register_all_tools()
    tools = await server.mcp.list_tools()
    print(f"Registered {len(tools)} tools:")
    for t in tools:
        print(f"  - {t.name}: {t.description.splitlines()[0] if t.description else ''}")


if __name__ == "__main__":
    asyncio.run(main())
