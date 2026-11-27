param([switch]$Public)

$ErrorActionPreference = 'Stop'
if ([IntPtr]::Size -ne 4 -or $PSVersionTable.PSEdition -eq 'Core') {
    $installerTestHost = Join-Path $env:WINDIR 'SysWOW64\WindowsPowerShell\v1.0\powershell.exe'
    if ($Public) {
        & $installerTestHost -NoProfile -ExecutionPolicy Bypass -File $PSCommandPath -Public
    } else {
        & $installerTestHost -NoProfile -ExecutionPolicy Bypass -File $PSCommandPath
    }
    if ($LASTEXITCODE -ne 0) { throw 'x86 installer tests failed.' }
    return
}
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..'))
$env:FFX_PRISM_TEST_LOCK = Join-Path $repo 'prism.lock.json'
if (-not $Public -and -not $env:FFX_PRISM_TEST_PACKAGE) {
    $prismLock = Get-Content -LiteralPath $env:FFX_PRISM_TEST_LOCK -Raw | ConvertFrom-Json
    $packagePath = 'packages\' + $prismLock.package_id + '\runtime'
    $prismRoots = @(
        $env:PRISM_ROOT
        (Join-Path $repo 'third_party\prism')
        (Join-Path $repo '..\..\..\codex\prism')
    )
    foreach ($root in $prismRoots) {
        if (-not $root) { continue }
        $candidate = Join-Path $root $packagePath
        if (Test-Path -LiteralPath $candidate -PathType Container) {
            $env:FFX_PRISM_TEST_PACKAGE = $candidate
            break
        }
    }
}
if (-not $Public) {
    if (-not $env:FFX_PRISM_TEST_PACKAGE -or -not (Test-Path -LiteralPath $env:FFX_PRISM_TEST_PACKAGE -PathType Container)) {
        throw 'The locked PRISM test runtime is missing. Set FFX_PRISM_TEST_PACKAGE to its runtime directory.'
    }
    $env:FFX_PRISM_TEST_PACKAGE = [IO.Path]::GetFullPath($env:FFX_PRISM_TEST_PACKAGE)
}
$source = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$fixture = Join-Path $repo ('install\windows-installer-tests-' + [guid]::NewGuid().ToString('N'))
[void][IO.Directory]::CreateDirectory($fixture)
$compiler = Join-Path $env:WINDIR 'Microsoft.NET\Framework\v4.0.30319\csc.exe'
$test = Join-Path $fixture 'InstallerTests.exe'
$productionSources = @(Get-ChildItem -LiteralPath $source -Filter '*.cs' | ForEach-Object { $_.FullName })
$testSources = @(Get-ChildItem -LiteralPath $PSScriptRoot -Filter '*.cs' | ForEach-Object { $_.FullName })
& $compiler /nologo /target:exe /platform:x86 /warnaserror+ /main:InstallerTests /reference:System.Windows.Forms.dll /reference:System.Drawing.dll /reference:System.Web.Extensions.dll /reference:System.IO.Compression.dll ("/out:$test") @testSources @productionSources
if ($LASTEXITCODE -ne 0) { throw 'Installer test compilation failed.' }
$testArgs = @($fixture)
if ($Public) { $testArgs += '--public' }
& $test @testArgs
if ($LASTEXITCODE -ne 0) { throw 'Synthetic installer tests failed.' }
# Compile the actual GitHub-only setup, including its pinned stock inventories.
$setup = Join-Path $fixture 'InstallFFXScreenReader.exe'
& (Join-Path $source 'build_installer.ps1') -OutputPath $setup
if ($Public) {
    Write-Output 'SKIP: locked PRISM installation, migration, rollback and release ZIP checks require the full installer suite.'
    return
}
# Package fake x86 headers and the real locked PRISM fixture as a hosted ZIP.
# None of the native files or synthetic executables are executed.
Import-Module (Join-Path $source 'release_payload.psm1') -Force
$nativeDirectory = Join-Path $fixture 'synthetic-native'
[void][IO.Directory]::CreateDirectory($nativeDirectory)
$fixtureReadme = Join-Path $nativeDirectory 'README.md'
$sources = Get-FfxPayloadSources $nativeDirectory (Join-Path $nativeDirectory 'DEPENDENCIES.md') $fixtureReadme
foreach ($entry in $sources.GetEnumerator()) {
    if ($entry.Key.EndsWith('.dll')) {
        $bytes = New-Object byte[] 256
        $bytes[0] = 0x4d; $bytes[1] = 0x5a; $bytes[60] = 64
        $bytes[64] = 0x50; $bytes[65] = 0x45; $bytes[68] = 0x4c; $bytes[69] = 1
        [IO.File]::WriteAllBytes($entry.Value, $bytes)
    } else { [IO.File]::WriteAllText($entry.Value, "Synthetic release file: $($entry.Key)") }
}
$version = '9.8.7.0'
$archive = Join-Path $fixture "ffx-x2-screen-reader-$version-payload.zip"
New-FfxPayloadArchive -Version $version -PayloadSources $sources -RuntimeDirectory $env:FFX_PRISM_TEST_PACKAGE -PrismLockPath $env:FFX_PRISM_TEST_LOCK -ArchivePath $archive
$payload = Read-FfxVerifiedPayload $setup $archive $version
foreach ($entry in $sources.GetEnumerator()) {
    if ([Convert]::ToBase64String($payload.Invoke($entry.Key)) -cne [Convert]::ToBase64String([IO.File]::ReadAllBytes($entry.Value))) {
        throw "Hosted payload differs from fixture source: $($entry.Key)"
    }
}
$proxy = [IO.File]::ReadAllBytes($sources['dinput8.dll'])
$proxy[100] = 1
[IO.File]::WriteAllBytes($sources['dinput8.dll'], $proxy)
[IO.File]::WriteAllText($fixtureReadme, 'README for the newer release.')
$nextVersion = '9.8.8.0'
$nextArchive = Join-Path $fixture "ffx-x2-screen-reader-$nextVersion-payload.zip"
New-FfxPayloadArchive -Version $nextVersion -PayloadSources $sources -RuntimeDirectory $env:FFX_PRISM_TEST_PACKAGE -PrismLockPath $env:FFX_PRISM_TEST_LOCK -ArchivePath $nextArchive
foreach ($game in (Join-Path $fixture 'online-short'), (Join-Path $fixture ('online-long\' + ('a' * 72) + '\' + ('b' * 72) + '\' + ('c' * 72)))) {
    & $test --online-package $setup $archive $version $nextArchive $nextVersion $game
    if ($LASTEXITCODE -ne 0) { throw 'Hosted payload engine verification failed.' }
}
Write-Output "PASS: separate setup and release ZIP, exact fixture bytes, production preflight and short/long-path update transactions."

# A corrupt copied baseline must not replace an existing setup.
$badSource = Join-Path $fixture 'corrupt-inventory-source'
[void][IO.Directory]::CreateDirectory((Join-Path $badSource 'baselines'))
foreach ($file in $productionSources) { Copy-Item -LiteralPath $file -Destination $badSource }
foreach ($name in @('build_installer.ps1', 'setup.manifest')) { Copy-Item -LiteralPath (Join-Path $source $name) -Destination $badSource }
foreach ($file in Get-ChildItem -LiteralPath (Join-Path $source 'baselines') -File) { Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $badSource 'baselines') }
[IO.File]::AppendAllText((Join-Path $badSource 'baselines/359871_1235377778411650212.txt'), 'corrupt fixture')
$previousOutput = Join-Path $fixture 'previous-output.exe'
[IO.File]::WriteAllText($previousOutput, 'previous output preserved')
$refused = $false
try { & (Join-Path $badSource 'build_installer.ps1') -OutputPath $previousOutput }
catch { $refused = $_.Exception.ToString().Contains('base-game inventory is damaged') }
if (-not $refused -or [IO.File]::ReadAllText($previousOutput) -ne 'previous output preserved') { throw 'Corrupt catalog was not refused safely.' }
Write-Output 'PASS: setup build refuses a corrupt pinned inventory and preserves existing output.'
