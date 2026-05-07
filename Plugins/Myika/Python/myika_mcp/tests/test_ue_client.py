"""Round-trip the framed JSON protocol against an in-process fake UE listener."""

from __future__ import annotations

import asyncio
import json
import struct

import pytest

from myika_mcp.settings import Settings
from myika_mcp.ue_client import MyikaUEClient, MyikaUEError


async def _read_frame(reader: asyncio.StreamReader) -> dict:
    header = await reader.readexactly(4)
    (length,) = struct.unpack(">I", header)
    body = await reader.readexactly(length)
    return json.loads(body.decode("utf-8"))


async def _write_frame(writer: asyncio.StreamWriter, message: dict) -> None:
    body = json.dumps(message).encode("utf-8")
    writer.write(struct.pack(">I", len(body)) + body)
    await writer.drain()


async def _serve_one(handler, port_holder: list[int]) -> None:
    async def on_client(reader: asyncio.StreamReader, writer: asyncio.StreamWriter) -> None:
        try:
            while True:
                request = await _read_frame(reader)
                response = handler(request)
                await _write_frame(writer, response)
        except asyncio.IncompleteReadError:
            pass
        finally:
            writer.close()
            await writer.wait_closed()

    server = await asyncio.start_server(on_client, host="127.0.0.1", port=0)
    sock = server.sockets[0]
    port_holder.append(sock.getsockname()[1])
    async with server:
        await server.serve_forever()


@pytest.fixture
async def fake_server(monkeypatch):
    port_holder: list[int] = []

    def handler(request: dict) -> dict:
        if request["tool"] == "ping":
            return {"id": request["id"], "ok": True, "result": {"pong": True}}
        if request["tool"] == "set_time_of_day":
            hours = request["args"]["hours"]
            if hours < 0 or hours > 24:
                return {
                    "id": request["id"],
                    "ok": False,
                    "error": {"code": "INVALID_ARGS", "message": "hours must be 0..24"},
                }
            return {"id": request["id"], "ok": True, "result": {"hours_set": hours}}
        return {
            "id": request["id"],
            "ok": False,
            "error": {"code": "UNKNOWN_TOOL", "message": request["tool"]},
        }

    task = asyncio.create_task(_serve_one(handler, port_holder))

    while not port_holder:
        await asyncio.sleep(0.01)

    monkeypatch.setenv("MYIKA_MCP_PORT", str(port_holder[0]))
    yield port_holder[0]

    task.cancel()
    try:
        await task
    except asyncio.CancelledError:
        pass


async def test_ping_round_trip(fake_server):
    client = MyikaUEClient(Settings.from_env())
    try:
        result = await client.call("ping")
        assert result == {"pong": True}
    finally:
        await client.close()


async def test_invalid_args_raises(fake_server):
    client = MyikaUEClient(Settings.from_env())
    try:
        with pytest.raises(MyikaUEError) as info:
            await client.call("set_time_of_day", {"hours": 99})
        assert info.value.code == "INVALID_ARGS"
    finally:
        await client.close()


async def test_set_time_of_day_ok(fake_server):
    client = MyikaUEClient(Settings.from_env())
    try:
        result = await client.call("set_time_of_day", {"hours": 17})
        assert result == {"hours_set": 17}
    finally:
        await client.close()


async def test_unknown_tool(fake_server):
    client = MyikaUEClient(Settings.from_env())
    try:
        with pytest.raises(MyikaUEError) as info:
            await client.call("non_existent")
        assert info.value.code == "UNKNOWN_TOOL"
    finally:
        await client.close()
