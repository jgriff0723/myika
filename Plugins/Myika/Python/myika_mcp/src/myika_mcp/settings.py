"""Runtime settings — env-var driven, loaded once at import."""

from __future__ import annotations

import os
from dataclasses import dataclass


@dataclass(frozen=True)
class Settings:
    host: str
    port: int
    request_timeout_s: float
    connect_timeout_s: float
    connect_max_retries: int
    connect_retry_base_s: float
    connect_retry_max_s: float

    @classmethod
    def from_env(cls) -> "Settings":
        return cls(
            host=os.getenv("MYIKA_MCP_HOST", "127.0.0.1"),
            port=int(os.getenv("MYIKA_MCP_PORT", "13379")),
            request_timeout_s=float(os.getenv("MYIKA_MCP_REQUEST_TIMEOUT_S", "10")),
            connect_timeout_s=float(os.getenv("MYIKA_MCP_CONNECT_TIMEOUT_S", "5")),
            connect_max_retries=int(os.getenv("MYIKA_MCP_CONNECT_RETRIES", "3")),
            connect_retry_base_s=float(os.getenv("MYIKA_MCP_CONNECT_RETRY_BASE_S", "0.5")),
            connect_retry_max_s=float(os.getenv("MYIKA_MCP_CONNECT_RETRY_MAX_S", "5")),
        )
