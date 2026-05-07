@echo off
REM Launch MCP Inspector against the Myika MCP server.
REM Requires Node.js / npx on PATH. The inspector spawns the stdio server,
REM lists tool schemas, and lets you invoke tools manually with form-validated
REM inputs — useful for debugging client config or new tool definitions.
setlocal
set HERE=%~dp0
npx -y @modelcontextprotocol/inspector "%HERE%\run_myika_mcp.bat" %*
