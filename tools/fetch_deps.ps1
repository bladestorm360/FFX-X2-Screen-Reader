<#
.SYNOPSIS
Downloads the build dependencies pinned in tools\deps.lock.json into third_party\.

.DESCRIPTION
Each archive is verified against its pinned SHA256 before it is unpacked. A
mismatch aborts without touching third_party\. Dependencies that are already
present are skipped unless -Force is given.

PRISM is not fetched here. It is pinned by prism.lock.json at the repository
root and is only needed to package the installer; the build and the offline
tests use the PRISM headers kept in the source tree.
#>
[CmdletBinding()]
param([switch]$Force)
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$lock = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'deps.lock.json') -Raw | ConvertFrom-Json
Add-Type -AssemblyName System.IO.Compression.FileSystem
$work = Join-Path ([IO.Path]::GetTempPath()) ('ffxsr-deps-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $work | Out-Null
try {
    foreach ($prop in $lock.dependencies.PSObject.Properties) {
        $name = $prop.Name; $dep = $prop.Value
        $dest = Join-Path $repo ($dep.destination -replace '/', '\')
        $sentinel = Join-Path $dest ($dep.sentinel -replace '/', '\')
        if (-not $Force -and (Test-Path -LiteralPath $sentinel)) {
            Write-Output "${name} $($dep.version): already present at $dest"
            continue
        }
        $zip = Join-Path $work "$name.zip"
        Write-Output "${name} $($dep.version): downloading $($dep.url)"
        Invoke-WebRequest -Uri $dep.url -OutFile $zip -UseBasicParsing
        $stream = [IO.File]::OpenRead($zip)
        try { $actual = [BitConverter]::ToString([Security.Cryptography.SHA256]::Create().ComputeHash($stream)).Replace('-', '').ToLowerInvariant() } finally { $stream.Dispose() }
        if ($actual -ne $dep.sha256.ToLowerInvariant()) {
            throw "SHA256 mismatch for ${name}: expected $($dep.sha256), got $actual. Nothing was unpacked."
        }
        Write-Output "${name} $($dep.version): SHA256 verified $actual"
        $extract = Join-Path $work "$name-x"
        [IO.Compression.ZipFile]::ExtractToDirectory($zip, $extract)
        $source = $extract
        if ($dep.stripRoot) {
            $children = @(Get-ChildItem -LiteralPath $extract)
            if ($children.Count -ne 1 -or -not $children[0].PSIsContainer) { throw "Unexpected archive layout for $name." }
            $source = $children[0].FullName
        }
        if (-not (Test-Path -LiteralPath (Join-Path $source ($dep.sentinel -replace '/', '\')))) { throw "Archive for $name lacks $($dep.sentinel)." }
        if (Test-Path -LiteralPath $dest) { Remove-Item -LiteralPath $dest -Recurse -Force }
        New-Item -ItemType Directory -Force -Path (Split-Path $dest) | Out-Null
        Move-Item -LiteralPath $source -Destination $dest
        Write-Output "${name} $($dep.version): installed to $dest"
    }
} finally {
    Remove-Item -LiteralPath $work -Recurse -Force -ErrorAction SilentlyContinue
}
