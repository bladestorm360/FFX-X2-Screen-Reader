param([Parameter(Mandatory=$true)][string]$GameDirectory,
      [Parameter(Mandatory=$true)][string]$RepositoryRoot,
      [string]$TolkRoot = '', # Deprecated compatibility parameter; unused.
      [ValidateSet('Preflight','Installed')][string]$Phase = 'Preflight')
$ErrorActionPreference = 'Stop'
function Get-ArtifactHash([string]$Path) {
    # Keep deployment integrity available when Utility-module autoload is absent.
    # Hash the actual bytes with the same SHA256 algorithm as Get-FileHash.
    $algorithm = [Security.Cryptography.SHA256]::Create()
    $stream = $null
    try {
        $stream = [IO.File]::OpenRead($Path)
        return [BitConverter]::ToString($algorithm.ComputeHash($stream)).Replace('-','')
    } finally {
        if ($stream) { $stream.Dispose() }
        $algorithm.Dispose()
    }
}

$game = [IO.Path]::GetFullPath($GameDirectory)
$repo = [IO.Path]::GetFullPath($RepositoryRoot)
$stage = Join-Path $repo 'install\native'
. (Join-Path $repo 'src\speech\prism_dependency.ps1')
$prismLock = Get-PrismLock (Join-Path $repo 'prism.lock.json')
$null = Assert-PrismInstalled $game $prismLock
# Never replace modules in a running session: Windows can retain the old reader.
$running = @(Get-Process | Where-Object {
    $_.ProcessName -in @('FFX','FFX-2','FFX-2_LM','FFX&X-2_Will','FFX&X-2_LAUNCHER')
})
if ($running.Count) { throw ('Close the game and launcher before deploying: ' + (($running | Select-Object -ExpandProperty ProcessName) -join ', ')) }
$files = [ordered]@{
    'dinput8.dll' = Join-Path $repo 'src\proxy\dinput8.dll'
    'SDL3.dll' = Join-Path $stage 'SDL3.dll'
    'FFX_SR_SDL3_LICENSE.txt' = Join-Path $stage 'SDL3_LICENSE.txt'
    'MinHook.x86.dll' = Join-Path $stage 'MinHook.x86.dll'
    'FFXLauncherReader.dll' = Join-Path $stage 'FFXLauncherReader.dll'
    'FFXLauncherNative.dll' = Join-Path $stage 'FFXLauncherNative.dll'
    'FFX_SR_Native_Dependencies.md' = Join-Path $repo 'src\native\DEPENDENCIES.md'
}
function Assert-X86([string]$Path) {
    $bytes = [IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 64 -or [BitConverter]::ToUInt16($bytes,0) -ne 0x5a4d) { throw "Invalid PE: $Path" }
    $offset = [BitConverter]::ToInt32($bytes,0x3c)
    if ($offset -lt 64 -or $offset + 26 -gt $bytes.Length -or
        [BitConverter]::ToUInt32($bytes,$offset) -ne 0x4550 -or
        [BitConverter]::ToUInt16($bytes,$offset + 4) -ne 0x14c -or
        [BitConverter]::ToUInt16($bytes,$offset + 24) -ne 0x10b) { throw "Expected x86 PE32: $Path" }
}
foreach ($entry in $files.GetEnumerator()) {
    if (-not (Test-Path -LiteralPath $entry.Value -PathType Leaf)) { throw ('Required deployment artifact missing: ' + $entry.Value) }
    if ($entry.Key.EndsWith('.dll')) { Assert-X86 $entry.Value }
}
$stagedProxy = Join-Path $stage 'dinput8.dll'
if ((Get-ArtifactHash -Path $stagedProxy) -ne (Get-ArtifactHash -Path $files['dinput8.dll'])) {
    throw 'Proxy and staged native DLL differ; run the canonical build before deploying.'
}
$config = Join-Path $game 'FFX&X-2_LAUNCHER.exe.config'
$expectedManager = [ordered]@{
    appDomainManagerAssembly = 'FFXLauncherReader, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null'
    appDomainManagerType = 'FfxLauncher.ReaderDomainManager'
}
$document = New-Object Xml.XmlDocument
if (Test-Path -LiteralPath $config) {
    $document.Load($config)
    if ($document.DocumentElement.Name -ne 'configuration') { throw 'Unexpected launcher configuration root.' }
} elseif ($Phase -eq 'Installed') { throw 'Launcher bootstrap configuration is missing.' }
foreach ($entry in $expectedManager.GetEnumerator()) {
    $node = $document.SelectSingleNode('/configuration/runtime/' + $entry.Key)
    if (($null -ne $node -and $node.GetAttribute('value') -ne $entry.Value) -or
        ($Phase -eq 'Installed' -and $null -eq $node)) { throw ('Launcher bootstrap configuration mismatch: ' + $entry.Key) }
}
$verifier = Join-Path $stage 'profile_preflight.exe'
Assert-X86 $verifier
$executable = Join-Path $game 'FFX.exe'
$proxy = if ($Phase -eq 'Preflight') { $stagedProxy } else { Join-Path $game 'dinput8.dll' }
if ($Phase -eq 'Installed') {
    foreach ($entry in $files.GetEnumerator()) {
        $destination = Join-Path $game $entry.Key
        if ((Get-ArtifactHash -Path $entry.Value) -ne (Get-ArtifactHash -Path $destination)) {
            throw ('Deployment hash mismatch: ' + $entry.Key)
        }
    }
    Write-Output ('All ' + $files.Count + ' installed artifacts verified by SHA256; launcher bootstrap verified.')
}
& $verifier $proxy $executable
if ($LASTEXITCODE -ne 0) { throw "Native executable validation failed during $Phase; deployment cannot continue." }
& $verifier $proxy (Join-Path $game 'FFX&X-2_Will.exe')
if ($LASTEXITCODE -ne 0) { throw "Native Will caption validation failed during $Phase; deployment cannot continue." }
Write-Output ("Deployment $Phase checks passed.")
