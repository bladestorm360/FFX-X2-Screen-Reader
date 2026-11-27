param([Parameter(Mandatory=$true)][string]$InstallerPath,
      [Parameter(Mandatory=$true)][string]$ArchivePath,
      [Parameter(Mandatory=$true)][string]$Version,
      [Parameter(Mandatory=$true)][string]$EvidenceDirectory)
$ErrorActionPreference = 'Stop'
$InstallerPath = [IO.Path]::GetFullPath($InstallerPath)
$ArchivePath = [IO.Path]::GetFullPath($ArchivePath)
$EvidenceDirectory = [IO.Path]::GetFullPath($EvidenceDirectory)
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
if (-not $EvidenceDirectory.StartsWith($repo + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Security evidence must stay in the workspace.' }
if ([IntPtr]::Size -ne 4 -or $PSVersionTable.PSEdition -eq 'Core') {
    & (Join-Path $env:WINDIR 'SysWOW64\WindowsPowerShell\v1.0\powershell.exe') -NoProfile -NonInteractive -ExecutionPolicy RemoteSigned -File $PSCommandPath -InstallerPath $InstallerPath -ArchivePath $ArchivePath -Version $Version -EvidenceDirectory $EvidenceDirectory
    if ($LASTEXITCODE -ne 0) { throw 'Distribution security verification failed. No release is approved by this check.' }
    return
}
Import-Module (Join-Path $PSScriptRoot 'release_security.psm1') -Force
Import-Module (Join-Path $PSScriptRoot 'release_payload.psm1') -Force
if (Test-Path -LiteralPath $EvidenceDirectory) { throw 'Use a new evidence directory to preserve previous security results.' }
[void][IO.Directory]::CreateDirectory($EvidenceDirectory)
$receipt = [ordered]@{started_utc=[DateTime]::UtcNow.ToString('o');result='blocked';scanner=$null;status_before=$null;status_after=$null;artifacts=@();coverage='Exact standalone setup, payload ZIP, and all six DLLs from the verified ZIP.';security_preferences_changed=$false;vendor_submission=$false}
try {
    $receipt.status_before = Get-ReleaseDefenderStatus
    $receipt.scanner = Get-ReleaseDefenderScanner $receipt.status_before
    $targets = New-Object 'System.Collections.Generic.List[object]'
    $targets.Add([pscustomobject]@{name='installer';path=$InstallerPath})
    $targets.Add([pscustomobject]@{name='archive';path=$ArchivePath})
    $payload = Read-FfxVerifiedPayload $InstallerPath $ArchivePath $Version
    $payloadDirectory = Join-Path $EvidenceDirectory 'payload-binaries'
    [void][IO.Directory]::CreateDirectory($payloadDirectory)
    foreach ($name in @('dinput8.dll','MinHook.x86.dll','SDL3.dll','FFXLauncherReader.dll','FFXLauncherNative.dll','PrismRuntime.prism.dll')) {
        $path = Join-Path $payloadDirectory $name
        [IO.File]::WriteAllBytes($path, $payload.Invoke($name))
        $targets.Add([pscustomobject]@{name=$name;path=$path})
    }
    foreach ($target in $targets) {
        $signature = Get-AuthenticodeSignature -LiteralPath $target.path
        if ($target.name -ne 'archive' -and $signature.Status -notin @('Valid','NotSigned')) {
            throw ('Invalid or unverifiable executable signature: ' + $target.name + ' (' + $signature.Status + ').')
        }
        $signer = if ($signature.SignerCertificate) { $signature.SignerCertificate.Subject } else { $null }
        $entry = [ordered]@{name=$target.name;path=$target.path;sha256=(Get-FfxFileSha256 $target.path);bytes=(Get-Item -LiteralPath $target.path).Length;signature_status=[string]$signature.Status;signer=$signer;started_utc=[DateTime]::UtcNow.ToString('o');exit_code=$null;result='blocked'}
        $receipt.artifacts += $entry
        # This custom-scan flag affects this invocation only: exclusions are
        # ignored, archives are inspected and detections are reported without
        # quarantining/removing release or source files. Real-time protection
        # remains enabled. Never replace this with a preference/exclusion change.
        $output = (& $receipt.scanner -Scan -ScanType 3 -File $target.path -DisableRemediation -ReturnHR 2>&1 | Out-String)
        $entry.exit_code = $LASTEXITCODE
        $entry.finished_utc = [DateTime]::UtcNow.ToString('o')
        [IO.File]::WriteAllText((Join-Path $EvidenceDirectory ($target.name + '.scan.txt')), $output, (New-Object Text.UTF8Encoding($false)))
        Assert-ReleaseDefenderScanResult $entry.exit_code $output
        if ((Get-FfxFileSha256 $target.path) -cne $entry.sha256) { throw 'A scanned artifact changed during verification.' }
        $entry.result = 'no_detection_reported'
        Write-Output ('PASS: Defender custom scan: ' + $target.name)
    }
    $receipt.status_after = Get-ReleaseDefenderStatus
    foreach ($name in @('AMProductVersion','AMEngineVersion','AntivirusSignatureVersion')) {
        if ($receipt.status_before.$name -cne $receipt.status_after.$name) { throw 'Defender changed versions during scanning; repeat with new evidence.' }
    }
    $receipt.result = 'no_detection_reported'
} catch {
    $receipt.failure = $_.Exception.Message
    throw
} finally {
    $receipt.finished_utc = [DateTime]::UtcNow.ToString('o')
    [IO.File]::WriteAllText((Join-Path $EvidenceDirectory 'security-verification.json'), ($receipt | ConvertTo-Json -Depth 8), (New-Object Text.UTF8Encoding($false)))
}
