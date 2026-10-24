# Release-only Defender validation. Never change preferences, exclusions,
# signatures, origin metadata or protection state. Tests exercise the refusal
# decisions separately from the real scanner; production has no skip switch.
Set-StrictMode -Version 2

function Assert-ReleaseDefenderReady($Status) {
    foreach ($name in @('AMRunningMode','AMProductVersion','AMEngineVersion','AntivirusEnabled',
        'RealTimeProtectionEnabled','AntivirusSignatureVersion','AntivirusSignatureLastUpdated','DefenderSignaturesOutOfDate')) {
        if ($null -eq $Status -or $null -eq $Status.PSObject.Properties[$name] -or $null -eq $Status.$name) {
            throw "Defender status is incomplete: $name. Release scanning is blocked."
        }
    }
    if ($Status.AMRunningMode -ne 'Normal' -or $Status.AntivirusEnabled -ne $true -or
        $Status.RealTimeProtectionEnabled -ne $true -or $Status.DefenderSignaturesOutOfDate -ne $false -or
        [string]::IsNullOrWhiteSpace($Status.AntivirusSignatureVersion) -or
        [string]::IsNullOrWhiteSpace($Status.AMEngineVersion)) {
        throw 'Active Defender with current definitions and real-time protection is required. No protection settings were changed.'
    }
}

function Assert-ReleaseDefenderScanResult([int]$ExitCode, [string]$Output) {
    # With remediation disabled, a detection cannot turn into a successful
    # remediation result. Errors, detections and missing output all fail closed.
    if ($ExitCode -ne 0 -or [string]::IsNullOrWhiteSpace($Output)) {
        throw "Defender did not complete a no-detection scan (exit $ExitCode). Review the retained scan output."
    }
}

function Get-ReleaseDefenderStatus {
    # The installer resource reader is x86. Read Defender through native x64
    # Windows PowerShell instead of PowerShell 7's compatibility/remoting route.
    $system = if ([Environment]::Is64BitOperatingSystem -and -not [Environment]::Is64BitProcess) { 'Sysnative' } else { 'System32' }
    $hostPath = Join-Path $env:WINDIR ($system + '\WindowsPowerShell\v1.0\powershell.exe')
    $json = & $hostPath -NoProfile -NonInteractive -Command 'Get-MpComputerStatus -ErrorAction Stop | Select-Object AMRunningMode,AMProductVersion,AMEngineVersion,AntivirusEnabled,RealTimeProtectionEnabled,AntivirusSignatureVersion,AntivirusSignatureLastUpdated,DefenderSignaturesOutOfDate | ConvertTo-Json -Compress'
    if ($LASTEXITCODE -ne 0) { throw 'Defender status is unavailable. Release scanning is blocked.' }
    $status = $json | ConvertFrom-Json
    Assert-ReleaseDefenderReady $status
    return $status
}

function Get-ReleaseDefenderScanner($Status) {
    $platform = Join-Path $env:ProgramData 'Microsoft\Windows Defender\Platform'
    $folders = @(Get-ChildItem -LiteralPath $platform -Directory | Where-Object {
        $_.Name -match '^\d+\.\d+\.\d+\.\d+-\d+$' -and $_.Name.StartsWith($Status.AMProductVersion + '-', [StringComparison]::Ordinal)
    } | Sort-Object Name -Descending)
    foreach ($folder in $folders) {
        $candidate = Join-Path $folder.FullName 'MpCmdRun.exe'
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            $signature = Get-AuthenticodeSignature -LiteralPath $candidate
            if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch 'O=Microsoft Corporation(?:,|$)') {
                throw 'The Defender scanner does not have a valid Microsoft signature.'
            }
            return $candidate
        }
    }
    throw 'The active Defender platform scanner could not be located.'
}

Export-ModuleMember -Function Assert-ReleaseDefenderReady,Assert-ReleaseDefenderScanResult,Get-ReleaseDefenderStatus,Get-ReleaseDefenderScanner
