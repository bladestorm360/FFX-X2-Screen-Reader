param([Parameter(Mandatory=$true)][string]$Version,
      [Parameter(Mandatory=$true)][string]$EvidenceDirectory)
$ErrorActionPreference = 'Stop'
if ($Version -cnotmatch '^[0-9]{1,5}(\.[0-9]{1,5}){1,3}$') { throw 'Expected a stable dotted release version.' }
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$EvidenceDirectory = [IO.Path]::GetFullPath($EvidenceDirectory)
if (-not $EvidenceDirectory.StartsWith($repo + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Evidence must stay in the workspace.' }
if ([IntPtr]::Size -ne 4 -or $PSVersionTable.PSEdition -eq 'Core') {
    & (Join-Path $env:WINDIR 'SysWOW64\WindowsPowerShell\v1.0\powershell.exe') -NoProfile -ExecutionPolicy Bypass -File $PSCommandPath -Version $Version -EvidenceDirectory $EvidenceDirectory
    if ($LASTEXITCODE -ne 0) { throw 'Final release verification failed.' }
    return
}
Import-Module (Join-Path $PSScriptRoot 'release_payload.psm1') -Force
function Hash-Bytes([byte[]]$Bytes) {
    $sha = [Security.Cryptography.SHA256]::Create()
    try { [BitConverter]::ToString($sha.ComputeHash($Bytes)).Replace('-', '').ToLowerInvariant() }
    finally { $sha.Dispose() }
}
$folder = Join-Path $repo ('releases\' + $Version)
$archive = Join-Path $folder ('ffx-x2-screen-reader-' + $Version + '-payload.zip')
$installer = Join-Path $folder 'InstallFFXScreenReader.exe'
$archiveHash = Get-FfxFileSha256 $archive
$installerHash = Get-FfxFileSha256 $installer
$payload = Read-FfxVerifiedPayload $installer $archive $Version
$native = Get-FfxNativeSources (Join-Path $repo 'install\native') (Join-Path $repo 'src\native\DEPENDENCIES.md')
$verified = [ordered]@{}
foreach ($entry in $native.GetEnumerator()) {
    $hash = Hash-Bytes ($payload.Invoke($entry.Key))
    if ($hash -cne (Get-FfxFileSha256 $entry.Value)) {
        throw "Release payload differs from the verified stage: $($entry.Key)"
    }
    $verified['install/' + $entry.Key] = $hash
}
if ($verified['install/dinput8.dll'] -cne (Get-FfxFileSha256 (Join-Path $repo 'src\proxy\dinput8.dll'))) { throw 'Proxy stage differs.' }
$lockPath = Join-Path $repo 'prism.lock.json'
$lockHash = Hash-Bytes ($payload.Invoke('prism.lock.json'))
if ($lockHash -cne (Get-FfxFileSha256 $lockPath)) { throw 'Payload runtime lock differs.' }
$verified['prism.lock.json'] = $lockHash
$lock = Get-Content -LiteralPath $lockPath -Raw | ConvertFrom-Json
foreach ($entry in $lock.files.PSObject.Properties) {
    $hash = Hash-Bytes ($payload.Invoke('PrismRuntime.' + $entry.Name))
    if ($hash -cne $entry.Value.sha256) { throw ('Locked runtime/license differs: ' + $entry.Name) }
    $verified['PrismRuntime/' + $entry.Name] = $hash
}
$number = [Version]$Version
$expectedVersion = '{0}.{1}.{2}.{3}' -f $number.Major,$number.Minor,[Math]::Max(0,$number.Build),[Math]::Max(0,$number.Revision)
$proxyVersion = [Diagnostics.FileVersionInfo]::GetVersionInfo($native['dinput8.dll']).FileVersion
if ($proxyVersion -cne $expectedVersion) { throw 'Proxy file version differs from release.' }
if ($archiveHash -cne (Get-FfxFileSha256 $archive) -or
    $installerHash -cne (Get-FfxFileSha256 $installer)) { throw 'Release files changed during verification.' }
$receipt = [ordered]@{version=$Version;zip=$archive;zip_bytes=(Get-Item -LiteralPath $archive).Length;zip_sha256=$archiveHash;installer_sha256=$installerHash;proxy_version=$proxyVersion;payload_hashes=$verified;game_executed=$false;registry_repair=$false}
[void][IO.Directory]::CreateDirectory($EvidenceDirectory)
$receipt | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $EvidenceDirectory 'package-verification.json') -Encoding UTF8
Write-Output ('PASS: standalone setup has only stock inventory resources; exact versioned payload ZIP, staged hashes and PRISM lock verified. SHA256 ' + $archiveHash)
