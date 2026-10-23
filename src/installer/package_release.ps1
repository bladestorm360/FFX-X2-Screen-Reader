param([string]$Version = '1.0.1',
      [string]$TolkRoot = '', # Deprecated compatibility parameter; unused.
      [string]$RuntimeDirectory,
      [Parameter(Mandatory=$true)][string]$SevenZip)
$ErrorActionPreference = 'Stop'
if ([IntPtr]::Size -ne 4 -or $PSVersionTable.PSEdition -eq 'Core') {
    $releaseBuildHost = Join-Path $env:WINDIR 'SysWOW64\WindowsPowerShell\v1.0\powershell.exe'
    $forward = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $PSCommandPath, '-Version', $Version, '-SevenZip', $SevenZip)
    if ($RuntimeDirectory) { $forward += @('-RuntimeDirectory', $RuntimeDirectory) }
    & $releaseBuildHost @forward
    if ($LASTEXITCODE -ne 0) { throw 'x86 release packaging failed.' }
    return
}
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
if ($Version -in @('.', '..') -or $Version.IndexOfAny([IO.Path]::GetInvalidFileNameChars()) -ge 0) { throw 'Invalid release version.' }
if (-not (Test-Path -LiteralPath $SevenZip -PathType Leaf)) { throw '7-Zip is required.' }
function Assert-WorkspacePath([string]$Path) {
    $full = [IO.Path]::GetFullPath($Path)
    if (-not $full.StartsWith($repo + '\', [StringComparison]::OrdinalIgnoreCase)) { throw "Path escapes workspace: $full" }
    return $full
}
$stamp = [DateTime]::Now.ToString('yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N').Substring(0,8)
$work = Assert-WorkspacePath (Join-Path $repo ('install\windows-release-' + $stamp))
$name = 'ffx-x2-screen-reader-' + $Version
$package = Join-Path $work $name
$payloadPackage = Join-Path $work 'payload'
$payload = Join-Path $payloadPackage 'install'
[void][IO.Directory]::CreateDirectory($package)
[void][IO.Directory]::CreateDirectory($payload)
$native = Join-Path $repo 'install\native'
$sources = [ordered]@{
    'dinput8.dll' = Join-Path $native 'dinput8.dll'
    'MinHook.x86.dll' = Join-Path $native 'MinHook.x86.dll'
    'SDL3.dll' = Join-Path $native 'SDL3.dll'
    'FFXLauncherReader.dll' = Join-Path $native 'FFXLauncherReader.dll'
    'FFXLauncherNative.dll' = Join-Path $native 'FFXLauncherNative.dll'
    'FFX_SR_SDL3_LICENSE.txt' = Join-Path $native 'SDL3_LICENSE.txt'
    'FFX_SR_Native_Dependencies.md' = Join-Path $repo 'src\native\DEPENDENCIES.md'
}
if ((Get-FileHash -LiteralPath $sources['dinput8.dll']).Hash -ne
    (Get-FileHash -LiteralPath (Join-Path $repo 'src\proxy\dinput8.dll')).Hash) { throw 'Native/proxy stages differ. Run the canonical test build first.' }
$sums = @()
foreach ($entry in $sources.GetEnumerator()) {
    $destination = Join-Path $payload $entry.Key
    Copy-Item -LiteralPath $entry.Value -Destination $destination
    $hash = (Get-FileHash -LiteralPath $entry.Value -Algorithm SHA256).Hash
    if ((Get-FileHash -LiteralPath $destination).Hash -ne $hash) { throw "Payload staging mismatch: $($entry.Key)" }
    $sums += $hash.ToLowerInvariant() + '  install/' + $entry.Key
}
[IO.File]::WriteAllLines((Join-Path $payloadPackage 'SHA256SUMS.txt'), $sums, (New-Object Text.UTF8Encoding($false)))
$exe = Join-Path $package 'InstallFFXScreenReader.exe'
& (Join-Path $PSScriptRoot 'build_installer.ps1') -PackageDirectory $payloadPackage -OutputPath $exe -RuntimeDirectory $RuntimeDirectory
$assembly = [Reflection.Assembly]::LoadFile($exe)
if (@($assembly.GetManifestResourceNames()).Count -ne 26) { throw 'Expected 7 native payloads, 15 PRISM/runtime licenses, source lock, checksum anchor and two pinned game inventories.' }
foreach ($entry in $sources.GetEnumerator()) {
    $stream = $assembly.GetManifestResourceStream('Payload.' + $entry.Key)
    $bytes = New-Object IO.MemoryStream
    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        $stream.CopyTo($bytes)
        $hash = [BitConverter]::ToString($sha.ComputeHash($bytes.ToArray())).Replace('-', '')
        if ($hash -ne (Get-FileHash -LiteralPath $entry.Value).Hash) { throw "Embedded source mismatch: $($entry.Key)" }
    } finally { $stream.Dispose(); $bytes.Dispose(); $sha.Dispose() }
}
$program = $assembly.GetType('FfxScreenReader.Setup.Program', $true)
$engine = $program.GetMethod('Engine', [Reflection.BindingFlags]'Static, NonPublic').Invoke($null, @())
Write-Output 'PASS: final installer parser, all 7 native source hashes and 15 locked PRISM/license resources.'
foreach ($doc in @(@('README.md','README.txt'), @('CHANGELOG.md','CHANGELOG.txt'), @('Docs\controls.md','CONTROLS.txt'))) {
    Copy-Item -LiteralPath (Join-Path $repo $doc[0]) -Destination (Join-Path $package $doc[1])
}
$sums = @()
foreach ($file in Get-ChildItem -LiteralPath $package -File | Sort-Object Name) {
    $sums += (Get-FileHash -LiteralPath $file.FullName).Hash.ToLowerInvariant() + '  ' + $file.Name
}
[IO.File]::WriteAllLines((Join-Path $package 'SHA256SUMS.txt'), $sums, (New-Object Text.UTF8Encoding($false)))
$zip = Join-Path $work ($name + '.zip')
Push-Location $work
try {
    & $SevenZip a -tzip -mx=9 $zip $name
    if ($LASTEXITCODE -ne 0) { throw 'Release archive creation failed.' }
    & $SevenZip t $zip
    if ($LASTEXITCODE -ne 0) { throw 'Release archive integrity check failed.' }
} finally { Pop-Location }
# Preserve the previous local release outside the refreshed distribution.
# Both resolved source/destination paths are checked before directory moves.
$release = Assert-WorkspacePath (Join-Path $repo ('releases\' + $Version))
$oldPackage = Assert-WorkspacePath (Join-Path $release $name)
$oldZip = Assert-WorkspacePath (Join-Path $release ($name + '.zip'))
$backup = Assert-WorkspacePath (Join-Path $repo ('install\release-backups\' + $Version + '--' + $stamp))
[void][IO.Directory]::CreateDirectory($release)
[void][IO.Directory]::CreateDirectory($backup)
if (Test-Path -LiteralPath $oldPackage) { Move-Item -LiteralPath $oldPackage -Destination (Join-Path $backup $name) }
if (Test-Path -LiteralPath $oldZip) { Move-Item -LiteralPath $oldZip -Destination (Join-Path $backup ($name + '.zip')) }
Move-Item -LiteralPath (Assert-WorkspacePath $package) -Destination $oldPackage
Move-Item -LiteralPath (Assert-WorkspacePath $zip) -Destination $oldZip
Write-Output "PASS: installer/document ZIP prepared: $oldZip"
Write-Output "Previous release retained: $backup"
