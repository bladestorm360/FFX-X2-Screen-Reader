<#
.SYNOPSIS
Downloads the build dependencies pinned in tools\deps.lock.json into third_party\.

.DESCRIPTION
Each archive is verified against its pinned SHA256 before it is unpacked.
Previously verified dependency directories are reused.

PRISM is not fetched here. It is pinned by prism.lock.json at the repository
root and is only needed to package the installer; the build and the offline
tests use the PRISM headers kept in the source tree.
#>
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$lock = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'deps.lock.json') -Raw | ConvertFrom-Json
Add-Type -AssemblyName System.IO.Compression.FileSystem
$cache = Join-Path $repo 'third_party'
$work = Join-Path $cache ('.fetch-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $work | Out-Null
try {
    foreach ($prop in $lock.dependencies.PSObject.Properties) {
        $name = $prop.Name; $dep = $prop.Value
        $dest = [IO.Path]::GetFullPath((Join-Path $repo ($dep.destination -replace '/', '\')))
        if ([IO.Path]::GetDirectoryName($dest) -ine $cache) { throw "Invalid dependency destination: $dest" }
        $sentinel = Join-Path $dest ($dep.sentinel -replace '/', '\')
        $receiptName = '.ffx-archive-sha256'
        $receipt = Join-Path $dest $receiptName
        if (Test-Path -LiteralPath $dest) {
            if ((Test-Path -LiteralPath $sentinel -PathType Leaf) -and
                (Test-Path -LiteralPath $receipt -PathType Leaf) -and
                ([IO.File]::ReadAllText($receipt) -eq $dep.sha256)) {
                Write-Output "${name} $($dep.version): using the previously verified archive at $dest"
                continue
            }
            throw "Unrecognized dependency directory: $dest. Move it aside before fetching this lock."
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
        [IO.File]::WriteAllText((Join-Path $source $receiptName), $actual)
        [IO.Directory]::Move($source, $dest)
        Write-Output "${name} $($dep.version): installed to $dest"
    }
} finally {
    Remove-Item -LiteralPath $work -Recurse -Force -ErrorAction SilentlyContinue
}
