# Prints the FINAL FANTASY FFX&FFX-2 HD Remaster folder and exits 0.
# Order: FFX_GAME_DIR (when set it is authoritative, never falls back), then
# every Steam library. Exits 1 with one ERROR line when nothing is found.
# Writes nothing to disk. FFX_STEAM_PATH overrides the registry (used by tests).
$ErrorActionPreference = 'Stop'
$name = 'FINAL FANTASY FFX&FFX-2 HD Remaster'

function Test-Game([string]$dir) {
    return $dir -and (Test-Path -LiteralPath (Join-Path $dir 'FFX.exe'))
}

if ($env:FFX_GAME_DIR) {
    if (Test-Game $env:FFX_GAME_DIR) {
        Write-Output $env:FFX_GAME_DIR
        exit 0
    }
    [Console]::Error.WriteLine("ERROR: FFX_GAME_DIR has no FFX.exe: $($env:FFX_GAME_DIR)")
    exit 1
}

$steamRoots = @()
if ($env:FFX_STEAM_PATH) {
    $steamRoots += $env:FFX_STEAM_PATH
} else {
    foreach ($key in @(
            @{ Path = 'HKCU:\Software\Valve\Steam'; Name = 'SteamPath' },
            @{ Path = 'HKLM:\SOFTWARE\WOW6432Node\Valve\Steam'; Name = 'InstallPath' })) {
        $value = (Get-ItemProperty -LiteralPath $key.Path -ErrorAction SilentlyContinue).($key.Name)
        # A stale registry value (no steamapps folder) is skipped.
        if ($value -and (Test-Path -LiteralPath (Join-Path $value 'steamapps'))) { $steamRoots += $value }
    }
}

$libraries = @()
foreach ($steam in $steamRoots) {
    $libraries += $steam
    $vdf = Join-Path $steam 'steamapps\libraryfolders.vdf'
    if (Test-Path -LiteralPath $vdf) {
        foreach ($m in [regex]::Matches((Get-Content -LiteralPath $vdf -Raw), '"path"\s+"([^"]+)"')) {
            $libraries += $m.Groups[1].Value.Replace('\\', '\')
        }
    }
}

foreach ($lib in ($libraries | Select-Object -Unique)) {
    $candidate = Join-Path $lib "steamapps\common\$name"
    if (Test-Game $candidate) {
        Write-Output ([System.IO.Path]::GetFullPath($candidate))
        exit 0
    }
}

[Console]::Error.WriteLine('ERROR: game folder not found. Set FFX_GAME_DIR.')
exit 1
