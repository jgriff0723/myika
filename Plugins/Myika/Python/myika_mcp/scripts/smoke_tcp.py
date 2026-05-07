"""Send framed JSON requests directly to the Myika MCP UE listener.

No MCP layer involved — just the raw TCP framing. Use this to verify the
C++ side is reachable before debugging Claude/Cursor configuration.

Usage:
    python smoke_tcp.py                   # sends `ping` + `get_global_state`
    python smoke_tcp.py set_time_of_day '{"hours": 17}'
"""

from __future__ import annotations

import json
import socket
import struct
import sys
import uuid
from typing import Any


HOST = "127.0.0.1"
PORT = 13379


def call(sock: socket.socket, tool: str, args: dict[str, Any] | None = None) -> dict[str, Any]:
    request_id = uuid.uuid4().hex
    body = json.dumps({"id": request_id, "tool": tool, "args": args or {}}).encode("utf-8")
    sock.sendall(struct.pack(">I", len(body)) + body)

    header = recv_exact(sock, 4)
    (length,) = struct.unpack(">I", header)
    response_body = recv_exact(sock, length)
    return json.loads(response_body.decode("utf-8"))


def recv_exact(sock: socket.socket, n: int) -> bytes:
    buf = bytearray()
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            raise ConnectionError("UE side closed the connection")
        buf.extend(chunk)
    return bytes(buf)


def main() -> int:
    with socket.create_connection((HOST, PORT), timeout=5) as sock:
        if len(sys.argv) > 1:
            tool = sys.argv[1]
            args = json.loads(sys.argv[2]) if len(sys.argv) > 2 else {}
            print(json.dumps(call(sock, tool, args), indent=2))
        else:
            print("--- ping ---")
            print(json.dumps(call(sock, "ping"), indent=2))
            print("--- get_global_state ---")
            print(json.dumps(call(sock, "get_global_state"), indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
