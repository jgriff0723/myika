"""Async TCP client speaking the Myika MCP framed-JSON protocol.

Wire format (matches the C++ MyikaMCPConnection):
    [4-byte big-endian length][UTF-8 JSON body]

The client maintains a single persistent connection per MyikaUEClient instance,
multiplexes requests by the `id` field, and surfaces errors as raised
MyikaUEError exceptions so MCP tool wrappers can simply `await call(...)`.
"""

from __future__ import annotations

import asyncio
import json
import socket
import struct
import uuid
from typing import Any, Awaitable, Optional

from .settings import Settings


class MyikaUEError(Exception):
    """Raised when the UE side returns an `ok: false` response."""

    def __init__(self, code: str, message: str) -> None:
        super().__init__(f"{code}: {message}")
        self.code = code
        self.message = message


class MyikaUEClient:
    """One TCP connection to the UE-side MyikaMCP listener."""

    def __init__(self, settings: Settings) -> None:
        self._settings = settings
        self._reader: Optional[asyncio.StreamReader] = None
        self._writer: Optional[asyncio.StreamWriter] = None
        self._lock = asyncio.Lock()
        self._pending: dict[str, asyncio.Future[dict[str, Any]]] = {}
        self._read_task: Optional[asyncio.Task[None]] = None
        self._closing = False

    async def connect(self) -> None:
        async with self._lock:
            if self._writer is not None and not self._writer.is_closing():
                return
            await self._connect_with_retries()
            self._closing = False
            self._read_task = asyncio.create_task(self._read_loop(), name="myika-mcp-read")

    async def _connect_with_retries(self) -> None:
        """Open the TCP connection with exponential backoff.

        Self-heals when Claude Desktop / Cursor spawns this server before the
        editor's MCP toolbar Start has been clicked. Pattern borrowed from
        flopperam/unreal-engine-mcp.
        """
        last_error: Optional[BaseException] = None
        for attempt in range(self._settings.connect_max_retries + 1):
            try:
                self._reader, self._writer = await asyncio.wait_for(
                    asyncio.open_connection(self._settings.host, self._settings.port),
                    timeout=self._settings.connect_timeout_s,
                )
                # TCP_NODELAY for low per-tool latency, SO_KEEPALIVE so a hung
                # editor surfaces as a closed socket rather than a dangling read.
                sock = self._writer.get_extra_info("socket")
                if sock is not None:
                    try:
                        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                        sock.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
                    except OSError:
                        pass  # not fatal — kernels vary
                return
            except (OSError, asyncio.TimeoutError) as exc:
                last_error = exc
                if attempt >= self._settings.connect_max_retries:
                    break
                delay = min(
                    self._settings.connect_retry_base_s * (2 ** attempt),
                    self._settings.connect_retry_max_s,
                )
                await asyncio.sleep(delay)

        message = (
            f"could not connect to UE listener at "
            f"{self._settings.host}:{self._settings.port} after "
            f"{self._settings.connect_max_retries + 1} attempt(s): {last_error}"
        )
        raise MyikaUEError("INTERNAL", message)

    async def close(self) -> None:
        self._closing = True
        if self._writer is not None:
            try:
                self._writer.close()
                await self._writer.wait_closed()
            except Exception:
                pass
        if self._read_task is not None:
            self._read_task.cancel()
            try:
                await self._read_task
            except (asyncio.CancelledError, Exception):
                pass
        self._reader = None
        self._writer = None
        self._read_task = None
        for fut in self._pending.values():
            if not fut.done():
                fut.set_exception(MyikaUEError("INTERNAL", "client shutting down"))
        self._pending.clear()

    async def call(self, tool: str, args: Optional[dict[str, Any]] = None) -> dict[str, Any]:
        """Send a request, await the matching response. Returns the `result` dict on success."""
        await self.connect()
        request_id = uuid.uuid4().hex
        payload = {"id": request_id, "tool": tool, "args": args or {}}

        loop = asyncio.get_running_loop()
        future: asyncio.Future[dict[str, Any]] = loop.create_future()
        self._pending[request_id] = future

        body = json.dumps(payload, separators=(",", ":")).encode("utf-8")
        frame = struct.pack(">I", len(body)) + body

        assert self._writer is not None
        self._writer.write(frame)
        try:
            await self._writer.drain()
        except Exception as exc:
            self._pending.pop(request_id, None)
            await self._fail_all(MyikaUEError("INTERNAL", f"send failed: {exc}"))
            raise

        try:
            response = await asyncio.wait_for(future, timeout=self._settings.request_timeout_s)
        finally:
            self._pending.pop(request_id, None)

        if not response.get("ok"):
            err = response.get("error") or {}
            raise MyikaUEError(
                code=str(err.get("code", "INTERNAL")),
                message=str(err.get("message", "unknown error")),
            )
        return response.get("result") or {}

    async def _read_loop(self) -> None:
        assert self._reader is not None
        try:
            while not self._closing:
                header = await self._reader.readexactly(4)
                (length,) = struct.unpack(">I", header)
                if length == 0 or length > 8 * 1024 * 1024:
                    await self._fail_all(MyikaUEError("BAD_FRAME", f"absurd length {length}"))
                    return
                body = await self._reader.readexactly(length)
                try:
                    message = json.loads(body.decode("utf-8"))
                except (UnicodeDecodeError, json.JSONDecodeError) as exc:
                    await self._fail_all(MyikaUEError("BAD_FRAME", f"unparseable response: {exc}"))
                    return
                self._dispatch(message)
        except asyncio.IncompleteReadError:
            await self._fail_all(MyikaUEError("INTERNAL", "connection closed by UE"))
        except asyncio.CancelledError:
            raise
        except Exception as exc:
            await self._fail_all(MyikaUEError("INTERNAL", f"read loop error: {exc}"))

    def _dispatch(self, message: dict[str, Any]) -> None:
        request_id = message.get("id")
        if isinstance(request_id, str):
            future = self._pending.get(request_id)
            if future is not None and not future.done():
                future.set_result(message)
                return
        # Unsolicited event (e.g. global_state_changed) — quietly ignored for MVP.

    async def _fail_all(self, error: MyikaUEError) -> None:
        for fut in self._pending.values():
            if not fut.done():
                fut.set_exception(error)
        self._pending.clear()


_singleton: Optional[MyikaUEClient] = None


def get_client() -> MyikaUEClient:
    global _singleton
    if _singleton is None:
        _singleton = MyikaUEClient(Settings.from_env())
    return _singleton


async def call(tool: str, args: Optional[dict[str, Any]] = None) -> dict[str, Any]:
    return await get_client().call(tool, args)


def call_sync(coro: Awaitable[dict[str, Any]]) -> dict[str, Any]:
    """Helper for environments that need a sync interface (tests)."""
    return asyncio.run(coro)  # type: ignore[arg-type]
