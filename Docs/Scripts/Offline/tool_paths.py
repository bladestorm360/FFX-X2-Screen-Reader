"""Path configuration for offline FFX tools."""

import os
from pathlib import Path

from evidence_paths import REPOSITORY


def game_directory() -> Path:
    configured = os.environ.get("FFX_GAME_DIR")
    if not configured:
        raise SystemExit("Set FFX_GAME_DIR to the authorized game directory.")
    return Path(configured).expanduser().resolve()


def ghidra_commands() -> list[str]:
    bootstrap = REPOSITORY.parents[2] / "codex/Enter-DevEnvironment.ps1"
    quoted = str(bootstrap).replace("'", "''")
    return [
        "$ErrorActionPreference = 'Stop'",
        f"if (-not $env:GHIDRA_HEADLESS) {{ . '{quoted}' }}",
        "if (-not (Test-Path -LiteralPath $env:GHIDRA_HEADLESS -PathType Leaf)) "
        "{ throw 'Set GHIDRA_HEADLESS to the Ghidra headless launcher.' }",
    ]
