$ErrorActionPreference = 'Stop'
if ([IntPtr]::Size -ne 4 -or $PSVersionTable.PSEdition -eq 'Core') {
    $installerTestHost = Join-Path $env:WINDIR 'SysWOW64\WindowsPowerShell\v1.0\powershell.exe'
    & $installerTestHost -NoProfile -ExecutionPolicy Bypass -File $PSCommandPath
    if ($LASTEXITCODE -ne 0) { throw 'x86 installer tests failed.' }
    return
}
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..'))
$env:FFX_PRISM_TEST_LOCK = Join-Path $repo 'prism.lock.json'
$env:FFX_PRISM_TEST_PACKAGE = 'D:\Games\Dev\codex\prism\packages\prism-0.18.3-b68e9c9-windows-x86-msvc-c17-1aa76c88\runtime'
$source = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$fixture = Join-Path $repo ('install\windows-installer-tests-' + [guid]::NewGuid().ToString('N'))
[void][IO.Directory]::CreateDirectory($fixture)
$compiler = Join-Path $env:WINDIR 'Microsoft.NET\Framework\v4.0.30319\csc.exe'
$test = Join-Path $fixture 'InstallerTests.exe'
$productionSources = @(Get-ChildItem -LiteralPath $source -Filter '*.cs' | ForEach-Object { $_.FullName })
$testSources = @(Get-ChildItem -LiteralPath $PSScriptRoot -Filter '*.cs' | ForEach-Object { $_.FullName })
& $compiler /nologo /target:exe /platform:x86 /warnaserror+ /main:InstallerTests /reference:System.Windows.Forms.dll /reference:System.Drawing.dll /reference:System.Web.Extensions.dll ("/out:$test") @testSources @productionSources
if ($LASTEXITCODE -ne 0) { throw 'Installer test compilation failed.' }
& $test $fixture
if ($LASTEXITCODE -ne 0) { throw 'Synthetic installer tests failed.' }
# Compile the real window with placeholder resources, without showing UI or running installation.
$ui = Join-Path $fixture 'InstallerUiCompileCheck.exe'
& $compiler /nologo /target:winexe /platform:x86 /warnaserror+ ("/out:$ui") /reference:System.Windows.Forms.dll /reference:System.Drawing.dll /reference:System.Web.Extensions.dll @productionSources
if ($LASTEXITCODE -ne 0) { throw 'Installer UI compilation failed.' }
Write-Output 'PASS: real WinForms installer source compiled with warnings as errors.'
# Build a self-contained EXE from fake x86 headers and inspect embedded resources.
# These synthetic files are never loaded as native modules or executed as game files.
$package = Join-Path $fixture 'synthetic-package'
$payloadDirectory = Join-Path $package 'install'
[void][IO.Directory]::CreateDirectory($payloadDirectory)
$names = @('dinput8.dll', 'MinHook.x86.dll', 'SDL3.dll', 'FFXLauncherReader.dll',
    'FFXLauncherNative.dll', 'FFX_SR_SDL3_LICENSE.txt', 'FFX_SR_Native_Dependencies.md')
$sums = @()
foreach ($name in $names) {
    $file = Join-Path $payloadDirectory $name
    if ($name.EndsWith('.dll')) {
        $bytes = New-Object byte[] 256
        $bytes[0] = 0x4d; $bytes[1] = 0x5a; $bytes[60] = 64
        $bytes[64] = 0x50; $bytes[65] = 0x45; $bytes[68] = 0x4c; $bytes[69] = 1
        [IO.File]::WriteAllBytes($file, $bytes)
    } else { [IO.File]::WriteAllText($file, "Synthetic resource: $name") }
    $sums += (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant() + '  install/' + $name
}
[IO.File]::WriteAllLines((Join-Path $package 'SHA256SUMS.txt'), $sums)
$embedded = Join-Path $fixture 'SyntheticSelfContainedSetup.exe'
& (Join-Path $source 'build_installer.ps1') -PackageDirectory $package -OutputPath $embedded -RuntimeDirectory $env:FFX_PRISM_TEST_PACKAGE
$assembly = [Reflection.Assembly]::LoadFile($embedded)
$resources = @($assembly.GetManifestResourceNames())
if ($resources.Count -ne 26) { throw 'Expected 7 native files, 15 PRISM/license files, checksum list, two base-game inventories, and Prism source lock.' }
foreach ($name in $names) {
    $stream = $assembly.GetManifestResourceStream('Payload.' + $name)
    $memory = New-Object IO.MemoryStream
    try {
        $stream.CopyTo($memory)
        $sha = [Security.Cryptography.SHA256]::Create()
        try { $hash = [BitConverter]::ToString($sha.ComputeHash($memory.ToArray())).Replace('-', '') }
        finally { $sha.Dispose() }
        if ($hash -ne (Get-FileHash -LiteralPath (Join-Path $payloadDirectory $name)).Hash) { throw "Embedded resource mismatch: $name" }
    } finally { $stream.Dispose(); $memory.Dispose() }
}
$lockStream = $assembly.GetManifestResourceStream('Payload.prism.lock.json')
if ($null -eq $lockStream) { throw 'Embedded Prism source lock is missing.' }
$lockMemory = New-Object IO.MemoryStream
try {
    $lockStream.CopyTo($lockMemory)
    if ([Convert]::ToBase64String($lockMemory.ToArray()) -ne [Convert]::ToBase64String([IO.File]::ReadAllBytes($env:FFX_PRISM_TEST_LOCK))) { throw 'Embedded Prism source lock mismatch.' }
} finally { $lockStream.Dispose(); $lockMemory.Dispose() }
& $test --embedded $embedded (Join-Path $fixture 'embedded-engine-game')
if ($LASTEXITCODE -ne 0) { throw 'Packaged engine verification failed.' }
$longGame = Join-Path $fixture ('embedded-long-path\' + ('a' * 72) + '\' + ('b' * 72) + '\' + ('c' * 72))
& $test --embedded $embedded $longGame
if ($LASTEXITCODE -ne 0) { throw 'Packaged engine long-path verification failed.' }
Write-Output 'PASS: self-contained resources, production preflight, payload transaction, and standard accessible controls.'
# Corrupt a fixture copy of the catalog; standalone build must retain an existing output.
$badSource = Join-Path $fixture 'corrupt-inventory-source'
[void][IO.Directory]::CreateDirectory((Join-Path $badSource 'baselines'))
foreach ($file in $productionSources) { Copy-Item -LiteralPath $file -Destination $badSource }
foreach ($name in @('build_installer.ps1', 'setup.manifest')) { Copy-Item -LiteralPath (Join-Path $source $name) -Destination $badSource }
foreach ($file in Get-ChildItem -LiteralPath (Join-Path $source 'baselines') -File) { Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $badSource 'baselines') }
[IO.File]::AppendAllText((Join-Path $badSource 'baselines/359871_1235377778411650212.txt'), 'corrupt fixture')
$previousOutput = Join-Path $fixture 'previous-output.exe'
[IO.File]::WriteAllText($previousOutput, 'previous output preserved')
$refused = $false
try { & (Join-Path $badSource 'build_installer.ps1') -PackageDirectory $package -OutputPath $previousOutput -PrismLockPath $env:FFX_PRISM_TEST_LOCK -RuntimeDirectory $env:FFX_PRISM_TEST_PACKAGE }
catch { $refused = $true }
if (-not $refused -or [IO.File]::ReadAllText($previousOutput) -ne 'previous output preserved') { throw 'Corrupt catalog replaced an existing build output.' }
Write-Output 'PASS: standalone build refuses a corrupt pinned inventory and preserves existing output.'
