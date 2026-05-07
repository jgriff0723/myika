@echo off
setlocal
set HERE=%~dp0
call "%HERE%\..\.venv\Scripts\activate.bat"
if errorlevel 1 (
    echo [myika-mcp] venv not found. Run scripts\setup_myika_mcp.bat first.
    exit /b 1
)
python -m myika_mcp %*
