"""Reuse-the-connection probe: two pings on one socket. If second hangs, the
C++ connection loop has a bug servicing back-to-back requests."""

from __future__ import annotations

import json
import socket
import struct
import sys
import uuid


HOST = "127.0.0.1"
PORT = 13379


def recv_exact(sock, n):
    buf = bytearray()
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            raise ConnectionError("closed")
        buf.extend(chunk)
    return bytes(buf)


def call(sock, tool):
    body = json.dumps({"id": uuid.uuid4().hex, "tool": tool, "args": {}}).encode("utf-8")
    sock.sendall(struct.pack(">I", len(body)) + body)
    (length,) = struct.unpack(">I", recv_exact(sock, 4))
    return json.loads(recv_exact(sock, length).decode("utf-8"))


def main() -> int:
    with socket.create_connection((HOST, PORT), timeout=5) as sock:
        print("--- ping 1 ---")
        print(json.dumps(call(sock, "ping"), indent=2))
        print("--- ping 2 (same socket) ---")
        print(json.dumps(call(sock, "ping"), indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
