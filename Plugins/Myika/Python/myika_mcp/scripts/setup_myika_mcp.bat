@echo off
setlocal
set HERE=%~dp0
pushd "%HERE%\.."

where uv >nul 2>nul
if %errorlevel%==0 (
    echo [myika-mcp] using uv to create .venv
    uv venv .venv --python 3.10
    if errorlevel 1 ( uv venv .venv --python 3.11 )
    if errorlevel 1 ( uv venv .venv --python 3.12 )
    call .venv\Scripts\activate.bat
    uv pip install -e .
    goto :done
)

echo [myika-mcp] uv not found, falling back to py launcher
py -3.10 -m venv .venv
if errorlevel 1 ( py -3.11 -m venv .venv )
if errorlevel 1 ( py -3.12 -m venv .venv )
if errorlevel 1 ( py -3 -m venv .venv )
if errorlevel 1 (
    echo [myika-mcp] could not find a Python 3.10+ interpreter; install one and retry.
    popd
    exit /b 1
)
call .venv\Scripts\activate.bat
python -m pip install --upgrade pip
pip install -e .

:done
popd
echo [myika-mcp] setup complete.
echo [myika-mcp] launch with: scripts\run_myika_mcp.bat
