@echo off
rem Resolve/validate the external evidence root before any export or test write.
rem This helper neither launches a game nor grants permission for a live probe.
set "FFX_EVIDENCE_VALIDATED="
for /f "delims=" %%I in ('python "%~dp0Offline\evidence_paths.py" --print-root') do set "FFX_EVIDENCE_VALIDATED=%%I"
if not defined FFX_EVIDENCE_VALIDATED (
    echo ERROR: Game evidence output must be outside every Git repository.
    exit /b 1
)
set "FFX_EVIDENCE_ROOT=%FFX_EVIDENCE_VALIDATED%"
set "FFX_EVIDENCE_VALIDATED="
exit /b 0
