$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot '..\release_security.psm1') -Force
$script:checks = 0
function Expect-Refusal([scriptblock]$Action) {
    $refused = $false
    try { & $Action } catch { $refused = $true }
    if (-not $refused) { throw 'An unsafe/incomplete scanner result was accepted.' }
    $script:checks++
}
$ready = [ordered]@{AMRunningMode='Normal';AMProductVersion='4.18.1.0';AMEngineVersion='1.1.1.0';AntivirusEnabled=$true;RealTimeProtectionEnabled=$true;AntivirusSignatureVersion='1.1.1.0';AntivirusSignatureLastUpdated=[DateTime]::UtcNow;DefenderSignaturesOutOfDate=$false}
Assert-ReleaseDefenderReady ([pscustomobject]$ready)
$script:checks++
foreach ($name in @($ready.Keys)) {
    $copy = [ordered]@{}
    foreach ($key in $ready.Keys) { if ($key -ne $name) { $copy[$key] = $ready[$key] } }
    Expect-Refusal { Assert-ReleaseDefenderReady ([pscustomobject]$copy) }
}
foreach ($change in @(@('AMRunningMode','Passive'),@('AntivirusEnabled',$false),@('RealTimeProtectionEnabled',$false),@('DefenderSignaturesOutOfDate',$true),@('AMEngineVersion',''),@('AntivirusSignatureVersion',''))) {
    $copy = [ordered]@{}
    foreach ($key in $ready.Keys) { $copy[$key] = $ready[$key] }
    $copy[$change[0]] = $change[1]
    Expect-Refusal { Assert-ReleaseDefenderReady ([pscustomobject]$copy) }
}
Assert-ReleaseDefenderScanResult 0 'Scan completed without a detection.'
$script:checks++
foreach ($code in @(2,-2147024891,-1)) { Expect-Refusal { Assert-ReleaseDefenderScanResult $code 'Scan did not succeed.' } }
Expect-Refusal { Assert-ReleaseDefenderScanResult 0 '' }
Write-Output "PASS: $script:checks release-security acceptance/refusal checks."
