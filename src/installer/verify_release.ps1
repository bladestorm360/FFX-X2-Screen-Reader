param([Parameter(Mandatory=$true)][string]$Version,
      [Parameter(Mandatory=$true)][string]$EvidenceDirectory)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$EvidenceDirectory = [IO.Path]::GetFullPath($EvidenceDirectory)
if (-not $EvidenceDirectory.StartsWith($repo + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Evidence must stay in the workspace.' }
if ([IntPtr]::Size -ne 4 -or $PSVersionTable.PSEdition -eq 'Core') {
    & (Join-Path $env:WINDIR 'SysWOW64\WindowsPowerShell\v1.0\powershell.exe') -NoProfile -ExecutionPolicy Bypass -File $PSCommandPath -Version $Version -EvidenceDirectory $EvidenceDirectory
    if ($LASTEXITCODE -ne 0) { throw 'Final release verification failed.' }
    return
}
function Hash-Stream($Stream) {
    $sha = [Security.Cryptography.SHA256]::Create()
    try { return [BitConverter]::ToString($sha.ComputeHash($Stream)).Replace('-', '').ToLowerInvariant() }
    finally { $sha.Dispose() }
}
function Hash-File([string]$Path) {
    $stream = [IO.File]::OpenRead($Path)
    try { return Hash-Stream $stream } finally { $stream.Dispose() }
}
$name = 'ffx-x2-screen-reader-' + $Version
$folder = Join-Path $repo ('releases\' + $Version + '\' + $name)
$archive = $folder + '.zip'
$installer = Join-Path $folder 'InstallFFXScreenReader.exe'
$assembly = [Reflection.Assembly]::LoadFile($installer)
$embedded = [ordered]@{}
foreach ($resource in $assembly.GetManifestResourceNames()) {
    $stream = $assembly.GetManifestResourceStream($resource)
    try { $embedded[$resource] = Hash-Stream $stream } finally { $stream.Dispose() }
}
if ($embedded.Count -ne 26) { throw 'Expected exactly26 installer resources.' }
$payload = [ordered]@{
    'dinput8.dll' = 'install\native\dinput8.dll'
    'MinHook.x86.dll' = 'install\native\MinHook.x86.dll'
    'SDL3.dll' = 'install\native\SDL3.dll'
    'FFXLauncherReader.dll' = 'install\native\FFXLauncherReader.dll'
    'FFXLauncherNative.dll' = 'install\native\FFXLauncherNative.dll'
    'FFX_SR_SDL3_LICENSE.txt' = 'install\native\SDL3_LICENSE.txt'
    'FFX_SR_Native_Dependencies.md' = 'src\native\DEPENDENCIES.md'
}
foreach ($entry in $payload.GetEnumerator()) {
    if ($embedded['Payload.' + $entry.Key] -cne (Hash-File (Join-Path $repo $entry.Value))) { throw ('Embedded payload differs: ' + $entry.Key) }
}
if ((Hash-File (Join-Path $repo 'src\proxy\dinput8.dll')) -cne $embedded['Payload.dinput8.dll']) { throw 'Proxy stage differs.' }
$lockPath = Join-Path $repo 'prism.lock.json'
$lock = Get-Content -LiteralPath $lockPath -Raw | ConvertFrom-Json
if ($embedded['Payload.prism.lock.json'] -cne (Hash-File $lockPath)) { throw 'Embedded runtime lock differs.' }
foreach ($entry in $lock.files.PSObject.Properties) {
    if ($embedded['Payload.PrismRuntime.' + $entry.Name] -cne $entry.Value.sha256) { throw ('Locked runtime/license differs: ' + $entry.Name) }
}
foreach ($catalog in @('1235377778411650212', '5168047039601173807')) {
    if ($embedded['Baseline.' + $catalog] -cne (Hash-File (Join-Path $PSScriptRoot ('baselines\359871_' + $catalog + '.txt')))) { throw 'Pinned stock inventory differs.' }
}
$documents = [ordered]@{'README.txt'='README.md';'CHANGELOG.txt'='CHANGELOG.md';'CONTROLS.txt'='Docs\controls.md'}
foreach ($entry in $documents.GetEnumerator()) {
    if ((Hash-File (Join-Path $folder $entry.Key)) -cne (Hash-File (Join-Path $repo $entry.Value))) { throw ('Player document differs: ' + $entry.Key) }
}
$expectedFiles = @('CHANGELOG.txt','CONTROLS.txt','InstallFFXScreenReader.exe','README.txt','SHA256SUMS.txt')
if (((Get-ChildItem -LiteralPath $folder -File).Name | Sort-Object) -join '|' -cne ($expectedFiles -join '|')) { throw 'Unexpected package file list.' }
$checksums = @{}
foreach ($line in [IO.File]::ReadAllLines((Join-Path $folder 'SHA256SUMS.txt'))) {
    if ($line -notmatch '^([a-f0-9]{64})  ([^/\\]+)$') { throw 'Invalid outer checksum line.' }
    if ($checksums.ContainsKey($Matches[2])) { throw 'Duplicate outer checksum.' }
    $checksums[$Matches[2]] = $Matches[1]
    if ((Hash-File (Join-Path $folder $Matches[2])) -cne $Matches[1]) { throw 'Outer checksum mismatch.' }
}
if ($checksums.Count -ne 4 -or $checksums.ContainsKey('SHA256SUMS.txt')) { throw 'Outer checksum inventory differs.' }
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [IO.Compression.ZipFile]::OpenRead($archive)
try {
    $files = @($zip.Entries | Where-Object { -not $_.FullName.EndsWith('/') })
    if ($files.Count -ne 5) { throw 'Unexpected ZIP payload count.' }
    foreach ($entry in $files) {
        if ($entry.FullName -cne ($name + '/' + $entry.Name) -or $entry.Name -notin $expectedFiles) { throw 'ZIP must contain one named root and the exact payload.' }
        $stream = $entry.Open()
        try { $zipHash = Hash-Stream $stream } finally { $stream.Dispose() }
        if ($zipHash -cne (Hash-File (Join-Path $folder $entry.Name))) { throw 'ZIP/folder bytes differ.' }
    }
} finally { $zip.Dispose() }
$parsedVersion = [Version]$Version
$expectedVersion = '{0}.{1}.{2}.{3}' -f $parsedVersion.Major,$parsedVersion.Minor,[Math]::Max(0,$parsedVersion.Build),[Math]::Max(0,$parsedVersion.Revision)
$setupVersion = [Diagnostics.FileVersionInfo]::GetVersionInfo($installer).FileVersion
$proxyVersion = [Diagnostics.FileVersionInfo]::GetVersionInfo((Join-Path $repo 'install\native\dinput8.dll')).FileVersion
if ($setupVersion -cne $expectedVersion -or $proxyVersion -cne $expectedVersion) { throw 'File version differs from release.' }
$receipt = [ordered]@{version=$Version;zip=$archive;zip_bytes=(Get-Item -LiteralPath $archive).Length;zip_sha256=(Hash-File $archive);installer_sha256=(Hash-File $installer);installer_version=$setupVersion;proxy_version=$proxyVersion;embedded_resources=$embedded;outer_checksums=$checksums;exact_zip_files=$expectedFiles;game_executed=$false;registry_repair=$false}
[void][IO.Directory]::CreateDirectory($EvidenceDirectory)
$receipt | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $EvidenceDirectory 'package-verification.json') -Encoding UTF8
Write-Output ('PASS: final EXE26 resources/pinned runtime/licenses, native versions, source documents, checksums and exact five-file ZIP; SHA256 ' + $receipt.zip_sha256)
