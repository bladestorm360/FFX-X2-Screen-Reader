Set-StrictMode -Version 2
$ErrorActionPreference = 'Stop'

function Get-FfxFileSha256([string]$Path) {
    $stream = [IO.File]::OpenRead($Path)
    try {
        $sha = [Security.Cryptography.SHA256]::Create()
        try { [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-', '').ToLowerInvariant() }
        finally { $sha.Dispose() }
    } finally { $stream.Dispose() }
}

function Get-FfxPayloadSources([string]$NativeDirectory, [string]$DependencyNotice, [string]$ReadmePath) {
    [ordered]@{
        'dinput8.dll' = Join-Path $NativeDirectory 'dinput8.dll'
        'MinHook.x86.dll' = Join-Path $NativeDirectory 'MinHook.x86.dll'
        'SDL3.dll' = Join-Path $NativeDirectory 'SDL3.dll'
        'FFXLauncherReader.dll' = Join-Path $NativeDirectory 'FFXLauncherReader.dll'
        'FFXLauncherNative.dll' = Join-Path $NativeDirectory 'FFXLauncherNative.dll'
        'FFX_SR_SDL3_LICENSE.txt' = Join-Path $NativeDirectory 'SDL3_LICENSE.txt'
        'FFX_SR_Native_Dependencies.md' = $DependencyNotice
        'FFX_SR_README.txt' = $ReadmePath
    }
}

function New-FfxPayloadArchive {
    param(
        [Parameter(Mandatory=$true)][string]$Version,
        [Parameter(Mandatory=$true)]$PayloadSources,
        [Parameter(Mandatory=$true)][string]$RuntimeDirectory,
        [Parameter(Mandatory=$true)][string]$PrismLockPath,
        [Parameter(Mandatory=$true)][string]$ArchivePath
    )
    if ($Version -cnotmatch '^[0-9]{1,5}(\.[0-9]{1,5}){1,3}$') { throw 'Expected a stable dotted release version.' }
    Add-Type -AssemblyName System.IO.Compression, System.IO.Compression.FileSystem
    $lock = Get-Content -LiteralPath $PrismLockPath -Raw | ConvertFrom-Json
    $runtimeFiles = @($lock.files.PSObject.Properties)
    if ($runtimeFiles.Count -ne 15) { throw 'Expected the locked 15-file PRISM runtime/license inventory.' }
    $archiveFiles = [ordered]@{'prism.lock.json' = $PrismLockPath}
    $checksums = @()
    foreach ($item in $PayloadSources.GetEnumerator()) {
        $entry = 'install/' + $item.Key
        $checksums += (Get-FfxFileSha256 $item.Value) + '  ' + $entry
        $archiveFiles[$entry] = $item.Value
    }
    foreach ($item in $runtimeFiles) {
        $name = $item.Name
        if ($name -cnotmatch '^[A-Za-z0-9_.-]+$' -or $name -in @('.', '..') -or $name -cne $item.Value.source) {
            throw "Invalid PRISM source filename: $name"
        }
        $file = Join-Path $RuntimeDirectory $name
        if ((Get-FfxFileSha256 $file) -cne $item.Value.sha256) {
            throw "Locked PRISM checksum failed: $name"
        }
        $archiveFiles['PrismRuntime/' + $name] = $file
    }
    $output = [IO.File]::Open($ArchivePath, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None)
    try {
        $zip = [IO.Compression.ZipArchive]::new($output, [IO.Compression.ZipArchiveMode]::Create, $true)
        try {
            foreach ($item in $archiveFiles.GetEnumerator()) {
                $null = [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip, $item.Value, $item.Key)
            }
            $metadata = [ordered]@{'release-version.txt' = 'v' + $Version; 'SHA256SUMS.txt' = $checksums -join "`n"}
            foreach ($item in $metadata.GetEnumerator()) {
                $stream = $zip.CreateEntry($item.Key).Open()
                try {
                    $bytes = [Text.Encoding]::UTF8.GetBytes($item.Value)
                    $stream.Write($bytes, 0, $bytes.Length)
                } finally { $stream.Dispose() }
            }
        } finally { $zip.Dispose() }
    } finally { $output.Dispose() }
}

function Read-FfxVerifiedPayload([string]$InstallerPath, [string]$ArchivePath, [string]$Version) {
    $assembly = [Reflection.Assembly]::Load([IO.File]::ReadAllBytes([IO.Path]::GetFullPath($InstallerPath)))
    $resources = @($assembly.GetManifestResourceNames() | Sort-Object)
    if (($resources -join '|') -cne 'Baseline.1235377778411650212|Baseline.5168047039601173807') {
        throw 'Setup must contain only the two stock-game inventory resources.'
    }
    $program = $assembly.GetType('FfxScreenReader.Setup.Program', $true)
    $verify = $program.GetMethod('VerifyReleaseArchive', [Reflection.BindingFlags]'Static, NonPublic')
    if ((Get-Item -LiteralPath $ArchivePath).Length -gt 64MB) { throw 'Release payload exceeds the archive size limit.' }
    $verify.Invoke($null, @([IO.File]::ReadAllBytes($ArchivePath), ('v' + $Version)))
}

Export-ModuleMember -Function Get-FfxFileSha256,Get-FfxPayloadSources,New-FfxPayloadArchive,Read-FfxVerifiedPayload
