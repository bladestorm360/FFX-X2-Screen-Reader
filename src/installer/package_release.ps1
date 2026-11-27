param([Parameter(Mandatory=$true)][string]$Version,
      [string]$RuntimeDirectory)
$ErrorActionPreference = 'Stop'
if ([IntPtr]::Size -ne 4 -or $PSVersionTable.PSEdition -eq 'Core') {
    $hostPath = Join-Path $env:WINDIR 'SysWOW64\WindowsPowerShell\v1.0\powershell.exe'
    $forward = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $PSCommandPath, '-Version', $Version)
    if ($RuntimeDirectory) { $forward += @('-RuntimeDirectory', $RuntimeDirectory) }
    & $hostPath @forward
    if ($LASTEXITCODE -ne 0) { throw 'x86 release packaging failed.' }
    return
}
if ($Version -cnotmatch '^[0-9]{1,5}(\.[0-9]{1,5}){1,3}$') { throw 'Expected a stable dotted release version.' }
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
Import-Module (Join-Path $PSScriptRoot 'release_payload.psm1') -Force
$lockPath = Join-Path $repo 'prism.lock.json'
if (-not $RuntimeDirectory) {
    $lock = Get-Content -LiteralPath $lockPath -Raw | ConvertFrom-Json
    $RuntimeDirectory = Join-Path (Join-Path $repo '..\..\..\codex\prism\packages') ($lock.package_id + '\runtime')
}
function Assert-WorkspacePath([string]$Path) {
    $full = [IO.Path]::GetFullPath($Path)
    if (-not $full.StartsWith($repo + '\', [StringComparison]::OrdinalIgnoreCase)) { throw "Path escapes workspace: $full" }
    return $full
}
$stamp = [DateTime]::Now.ToString('yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N').Substring(0,8)
$work = Assert-WorkspacePath (Join-Path $repo ('install\windows-release-' + $stamp))
[void][IO.Directory]::CreateDirectory($work)
$installer = Join-Path $work 'InstallFFXScreenReader.exe'
$assetName = 'ffx-x2-screen-reader-' + $Version + '-payload.zip'
$archive = Join-Path $work $assetName
$native = Get-FfxNativeSources (Join-Path $repo 'install\native') (Join-Path $repo 'src\native\DEPENDENCIES.md')
if ((Get-FfxFileSha256 $native['dinput8.dll']) -cne
    (Get-FfxFileSha256 (Join-Path $repo 'src\proxy\dinput8.dll'))) { throw 'Native/proxy stages differ. Run the canonical test build first.' }
$number = [Version]$Version
$expected = '{0}.{1}.{2}.{3}' -f $number.Major,$number.Minor,[Math]::Max(0,$number.Build),[Math]::Max(0,$number.Revision)
if ([Diagnostics.FileVersionInfo]::GetVersionInfo($native['dinput8.dll']).FileVersion -cne $expected) {
    throw 'The staged proxy version does not match the release version.'
}
& (Join-Path $PSScriptRoot 'build_installer.ps1') -OutputPath $installer
New-FfxPayloadArchive -Version $Version -NativeSources $native -RuntimeDirectory $RuntimeDirectory -PrismLockPath $lockPath -ArchivePath $archive
& (Join-Path $PSScriptRoot 'verify_distribution_security.ps1') -InstallerPath $installer -ArchivePath $archive -Version $Version -EvidenceDirectory (Join-Path $work 'security')
$release = Assert-WorkspacePath (Join-Path $repo ('releases\' + $Version))
$releaseInstaller = Assert-WorkspacePath (Join-Path $release 'InstallFFXScreenReader.exe')
$releaseArchive = Assert-WorkspacePath (Join-Path $release $assetName)
[void][IO.Directory]::CreateDirectory($release)
if ((Test-Path -LiteralPath $releaseInstaller) -or (Test-Path -LiteralPath $releaseArchive)) {
    $backup = Assert-WorkspacePath (Join-Path $repo ('install\release-backups\' + $Version + '--' + $stamp))
    [void][IO.Directory]::CreateDirectory($backup)
    foreach ($file in $releaseInstaller,$releaseArchive) {
        if (Test-Path -LiteralPath $file) { Move-Item -LiteralPath $file -Destination $backup }
    }
    Write-Output "Previous online release retained: $backup"
}
Move-Item -LiteralPath (Assert-WorkspacePath $installer) -Destination $releaseInstaller
Move-Item -LiteralPath (Assert-WorkspacePath $archive) -Destination $releaseArchive
Write-Output "PASS: standalone GitHub setup prepared: $releaseInstaller"
Write-Output "PASS: GitHub release payload prepared: $releaseArchive"
