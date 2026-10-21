"""Restore only the old global-side prefilter in a separate offline fixture."""
from pathlib import Path

root = Path(__file__).resolve().parents[3]
output = root / "install/inn-menu-followup-20261008/legacy_global_side"
output.mkdir(exist_ok=True)
native = root / "src/native"
source = (native / "nav_zones.cpp").read_text(encoding="utf8")
start = source.index("bool lineEndpoint(")
end = source.index("bool routeCrossesLine(", start)
previous = """bool crossesLine(const InteractionZone& zone,Point from,Point to) {
    const double x=zone.end.x-zone.center.x,z=zone.end.z-zone.center.z;
    const double a=x*(from.z-zone.center.z)-z*(from.x-zone.center.x);
    const double b=x*(to.z-zone.center.z)-z*(to.x-zone.center.x);
    const auto closest=onShape(zone,to);
    return (a*b<0 || (std::abs(a)<1e-8 && std::abs(b)>.1)) && distanceXZ(closest,to)<=1.01 &&
        (zone.height<=0 || std::abs(to.y-closest.y)<zone.height);
}
"""
source = source[:start] + previous + source[end:]
for zone, point in (("zone", "candidate"), ("zone", "p"), ("target.zone", "candidate")):
    source = source.replace(f"lineEndpoint({zone},{point})", f"crossesLine({zone},player,{point})")
(output / "nav_zones_original_side.cpp").write_text(source, encoding="utf8")
batch = f'''@echo off
setlocal
call "{root.parents[2] / 'dev_env.bat'}"
if not defined VCVARSALL exit /b 2
call "%VCVARSALL%" x86 >nul
if errorlevel 1 exit /b 2
pushd "%~dp0"
cl /nologo /W4 /EHsc /std:c++17 /MT /I"{native}" /Fe:prior_side_fixture.exe "{native / 'tests/navigation_fixture.cpp'}" "{native / 'atel.cpp'}" "{native / 'nav_geometry.cpp'}" nav_zones_original_side.cpp
if errorlevel 1 exit /b 2
prior_side_fixture.exe
set "FIXTURE_RESULT=%ERRORLEVEL%"
popd
exit /b %FIXTURE_RESULT%
'''
(output / "run_fixture.bat").write_bytes(batch.replace("\n", "\r\n").encode("utf8"))
print("Prepared isolated former-side fixture; production source unchanged.")
