param([Parameter(Mandatory=$true)][string]$PackageDirectory,
      [string]$OutputPath,
      [string]$RuntimeDirectory,
      [string]$PrismLockPath = (Join-Path $PSScriptRoot '..\..\prism.lock.json'))
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$package = [IO.Path]::GetFullPath($PackageDirectory)
if (-not $OutputPath) { $OutputPath = Join-Path $repo 'install\windows-installer\InstallFFXScreenReader.exe' }
$output = [IO.Path]::GetFullPath($OutputPath)
$PrismLockPath = [IO.Path]::GetFullPath($PrismLockPath)
$prismLock = Get-Content -LiteralPath $PrismLockPath -Raw | ConvertFrom-Json
if (-not $RuntimeDirectory) {
    $RuntimeDirectory = Join-Path (Join-Path $repo '..\..\..\codex\prism\packages') ($prismLock.package_id + '\runtime')
}
$RuntimeDirectory = [IO.Path]::GetFullPath($RuntimeDirectory)
if ([IntPtr]::Size -ne 4 -or $PSVersionTable.PSEdition -eq 'Core') {
    $modBuildHost = Join-Path $env:WINDIR 'SysWOW64\WindowsPowerShell\v1.0\powershell.exe'
    & $modBuildHost -NoProfile -ExecutionPolicy Bypass -File $PSCommandPath -PackageDirectory $package -OutputPath $output -PrismLockPath $PrismLockPath -RuntimeDirectory $RuntimeDirectory
    if ($LASTEXITCODE -ne 0) { throw 'x86 screen reader installer build failed.' }
    return
}
$compiler = Join-Path $env:WINDIR 'Microsoft.NET\Framework\v4.0.30319\csc.exe'
if (-not (Test-Path -LiteralPath $compiler -PathType Leaf)) { throw 'The Windows .NET Framework C# compiler is required.' }
$payload = @('dinput8.dll', 'MinHook.x86.dll', 'SDL3.dll', 'FFXLauncherReader.dll',
    'FFXLauncherNative.dll', 'FFX_SR_SDL3_LICENSE.txt',
    'FFX_SR_Native_Dependencies.md')
$hashes = @{}
foreach ($line in [IO.File]::ReadAllLines((Join-Path $package 'SHA256SUMS.txt'))) {
    if ($line -match '^([a-fA-F0-9]{64})\s+(.+)$') {
        $relative = $Matches[2].Replace('\', '/')
        if ($hashes.ContainsKey($relative)) { throw "Duplicate checksum: $relative" }
        $hashes[$relative] = $Matches[1].ToLowerInvariant()
    }
}
$manifest = New-Object 'System.Collections.Generic.List[string]'
$resources = @('/resource:' + $PrismLockPath + ',Payload.prism.lock.json')
foreach ($name in $payload) {
    $relative = 'install/' + $name
    $file = Join-Path $package $relative
    if (-not (Test-Path -LiteralPath $file -PathType Leaf) -or -not $hashes.ContainsKey($relative)) { throw "Incomplete package: $relative" }
    if ((Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant() -ne $hashes[$relative]) { throw "Package checksum failed: $relative" }
    if ($name.EndsWith('.dll')) {
        $bytes = [IO.File]::ReadAllBytes($file)
        if ($bytes.Length -lt 64 -or $bytes[0] -ne 0x4d -or $bytes[1] -ne 0x5a) { throw "Invalid PE file: $name" }
        $offset = [BitConverter]::ToInt32($bytes, 60)
        if ($offset -lt 0 -or $offset -gt ($bytes.Length - 6) -or [BitConverter]::ToUInt32($bytes, $offset) -ne 0x4550 -or
            [BitConverter]::ToUInt16($bytes, $offset + 4) -ne 0x14c) { throw "Expected x86 DLL: $name" }
    }
    $manifest.Add($hashes[$relative] + '  ' + $relative)
    $resources += ('/resource:' + $file + ',Payload.' + $name)
}
$prismFiles = @($prismLock.files.PSObject.Properties)
if ($prismFiles.Count -ne 15) { throw 'Expected the locked 15-file PRISM runtime/license inventory.' }
foreach ($entry in $prismFiles) {
    $name = $entry.Name
    if ($name -ne $entry.Value.source -or $name -in @('.', '..') -or $name.IndexOfAny([IO.Path]::GetInvalidFileNameChars()) -ge 0 -or $name.Contains('\') -or $name.Contains('/')) { throw 'Invalid flat PRISM source filename.' }
    $file = Join-Path $RuntimeDirectory $name
    if ((Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant() -ne $entry.Value.sha256) { throw "Locked PRISM checksum failed: $name" }
    $resources += ('/resource:' + $file + ',Payload.PrismRuntime.' + $name)
}
foreach ($id in @('1235377778411650212', '5168047039601173807')) {
    $resources += ('/resource:' + (Join-Path $PSScriptRoot ("baselines/359871_" + $id + ".txt")) + ',Baseline.' + $id)
}
[void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($output))
$build = Join-Path $repo ('install\windows-installer\build-' + [guid]::NewGuid().ToString('N'))
[void][IO.Directory]::CreateDirectory($build)
$embeddedSums = Join-Path $build 'SHA256SUMS.txt'
[IO.File]::WriteAllLines($embeddedSums, $manifest, (New-Object Text.UTF8Encoding($false)))
$temporary = Join-Path $build 'InstallFFXScreenReader.exe'
$arguments = @('/nologo', '/target:winexe', '/platform:x86', '/optimize+', '/warnaserror+',
    ('/out:' + $temporary), ('/win32manifest:' + (Join-Path $PSScriptRoot 'setup.manifest')),
    '/reference:System.Windows.Forms.dll', '/reference:System.Drawing.dll', '/reference:System.Web.Extensions.dll',
    ('/resource:' + $embeddedSums + ',Payload.SHA256SUMS.txt')) + $resources +
    @(Get-ChildItem -LiteralPath $PSScriptRoot -Filter '*.cs' | ForEach-Object { $_.FullName })
& $compiler @arguments
if ($LASTEXITCODE -ne 0) { throw 'Windows installer compilation failed.' }
# Validate pinned inventories in the compiled EXE before replacing an existing output.
$assembly = [Reflection.Assembly]::LoadFile($temporary)
$program = $assembly.GetType('FfxScreenReader.Setup.Program', $true)
$null = $program.GetMethod('Engine', [Reflection.BindingFlags]'Static, NonPublic').Invoke($null, @())
Copy-Item -LiteralPath $temporary -Destination $output -Force
Write-Output "PASS: self-contained x86 Windows installer, $($payload.Count) native payloads and $($prismFiles.Count) PRISM/license checksums verified: $output"
