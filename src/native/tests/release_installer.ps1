param([string]$Package)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..'))
$evidenceRoot = $env:FFX_EVIDENCE_ROOT
if ([string]::IsNullOrWhiteSpace($evidenceRoot)) { $evidenceRoot = Join-Path $repo '..\Analysis\FFX\RepositoryEvidence' }
$fixtureRoot = Join-Path ([IO.Path]::GetFullPath($evidenceRoot)) 'test-build'
if (-not $Package) { $Package = Join-Path $repo 'releases\0.5-Test Build\ffx-x2-screen-reader-0.5-Test Build' }
$fixture = Join-Path $fixtureRoot ('release-setup-tests-' + [guid]::NewGuid().ToString('N'))
$copy = Join-Path $fixture 'package'
[void][IO.Directory]::CreateDirectory($copy)
Copy-Item -LiteralPath (Join-Path $Package 'install') -Destination $copy -Recurse
foreach ($name in @('InstallLauncher.ps1', 'ConfigureLauncher.ps1', 'SHA256SUMS.txt')) {
    Copy-Item -LiteralPath (Join-Path $Package $name) -Destination $copy
}
$steam = Join-Path $fixture 'Steam'
$library = Join-Path $fixture 'Another drive & library [test]'
$game = Join-Path $library 'steamapps\common\Custom Game Name'
[void][IO.Directory]::CreateDirectory($game)
[void][IO.Directory]::CreateDirectory((Join-Path $steam 'steamapps'))
[IO.File]::WriteAllText((Join-Path $steam 'steamapps\libraryfolders.vdf'),
    ('"libraryfolders" { "0" { "path" "' + $steam.Replace('\', '\\') + '" } "1" { "path" "' + $library.Replace('\', '\\') + '" } }'))
[IO.File]::WriteAllText((Join-Path $library 'steamapps\appmanifest_359870.acf'), '"AppState" { "installdir" "Custom Game Name" }')
foreach ($name in @('FFX.exe', 'FFX&X-2_LAUNCHER.exe', 'frida-gadget.dll',
    'frida-gadget.config', 'frida-gadget.config.dll', 'ffx_sr.js', 'dinput8.dll', 'OtherMod.dll', 'FFXAccessibility.ini')) {
    [IO.File]::WriteAllText((Join-Path $game $name), ('original-' + $name))
}
$config = Join-Path $game 'FFX&X-2_LAUNCHER.exe.config'
$originalXml = '<configuration><runtime><gcConcurrent enabled="true"/></runtime><appSettings><add key="Preserved" value="yes"/></appSettings></configuration>'
[IO.File]::WriteAllText($config, $originalXml)
$setup = Join-Path $copy 'InstallLauncher.ps1'
& $setup -SteamDirectory $steam
& $setup -SteamDirectory $steam
foreach ($file in Get-ChildItem -LiteralPath (Join-Path $copy 'install') -File) {
    if ((Get-FileHash -LiteralPath $file.FullName).Hash -ne (Get-FileHash -LiteralPath (Join-Path $game $file.Name)).Hash) { throw 'Payload mismatch.' }
}
foreach ($name in @('frida-gadget.dll', 'frida-gadget.config', 'frida-gadget.config.dll', 'ffx_sr.js')) {
    if (Test-Path -LiteralPath (Join-Path $game $name)) { throw 'Legacy cleanup failed.' }
    $saved = @(Get-ChildItem -LiteralPath (Join-Path $game 'FFX_SR_Install_Backups') -Recurse -File | Where-Object Name -eq $name)
    if ($saved.Count -ne 1 -or [IO.File]::ReadAllText($saved[0].FullName) -ne ('original-' + $name)) { throw 'Legacy backup failed.' }
}
foreach ($name in @('OtherMod.dll', 'FFXAccessibility.ini')) {
    if ([IO.File]::ReadAllText((Join-Path $game $name)) -ne ('original-' + $name)) { throw 'Unrelated file changed.' }
}
if ([IO.File]::ReadAllText($config + '.ffx_sr_original') -ne $originalXml) { throw 'Original XML backup changed.' }
$xml = New-Object Xml.XmlDocument
$xml.Load($config)
if ($xml.SelectNodes('/configuration/runtime/appDomainManagerType').Count -ne 1 -or
    $xml.SelectSingleNode('/configuration/appSettings/add').GetAttribute('value') -ne 'yes') { throw 'XML preservation/repeat failed.' }

function Expect-Rejection([string]$Pattern) {
    $caught = $false
    try { & $setup -GameDirectory $game } catch {
        if ($_.ToString() -notmatch $Pattern) { throw }
        $caught = $true
    }
    if (-not $caught) { throw 'Expected rejection did not occur.' }
}
$before = [IO.File]::ReadAllText($config)
$tolk = Join-Path $copy 'install\Tolk.dll'
$tolkBytes = [IO.File]::ReadAllBytes($tolk)
[IO.File]::WriteAllText($tolk, 'corrupt')
Expect-Rejection 'checksum failed'
[IO.File]::WriteAllBytes($tolk, $tolkBytes)
Move-Item -LiteralPath $tolk -Destination ($tolk + '.held')
Expect-Rejection 'Incomplete release'
Move-Item -LiteralPath ($tolk + '.held') -Destination $tolk
if ([IO.File]::ReadAllText($config) -ne $before) { throw 'Failed preflight changed configuration.' }
[IO.File]::WriteAllText($config, '<configuration><runtime><appDomainManagerType value="Other.Manager"/></runtime></configuration>')
Expect-Rejection 'another AppDomainManager'
if ([IO.File]::ReadAllText($config) -notmatch 'Other.Manager') { throw 'Conflict overwritten.' }
[IO.File]::WriteAllText($config, $before)

# Force a failure after payload copying, using only this private fixture helper.
$helper = Join-Path $copy 'ConfigureLauncher.ps1'
[IO.File]::WriteAllText($helper, "throw 'Synthetic configuration failure'")
$sumFile = Join-Path $copy 'SHA256SUMS.txt'
$sumText = [IO.File]::ReadAllText($sumFile)
$helperHash = (Get-FileHash -LiteralPath $helper).Hash.ToLowerInvariant()
$sumText = [regex]::Replace($sumText, '(?m)^[a-fA-F0-9]{64}(\s+ConfigureLauncher\.ps1)\r?$', ($helperHash + '$1'))
[IO.File]::WriteAllText($sumFile, $sumText)
[IO.File]::WriteAllText((Join-Path $game 'dinput8.dll'), 'old-proxy')
[IO.File]::WriteAllText((Join-Path $game 'frida-gadget.dll'), 'old-gadget')
Remove-Item -LiteralPath (Join-Path $game 'Tolk.dll')
Expect-Rejection 'previous files were restored'
if ([IO.File]::ReadAllText((Join-Path $game 'dinput8.dll')) -ne 'old-proxy' -or
    [IO.File]::ReadAllText((Join-Path $game 'frida-gadget.dll')) -ne 'old-gadget' -or
    [IO.File]::ReadAllText($config) -ne $before -or (Test-Path -LiteralPath (Join-Path $game 'Tolk.dll'))) { throw 'Rollback failed.' }
Write-Output 'PASS: Steam library/manifest discovery, 13 payload hashes, legacy backups/cleanup, settings/XML preservation, repeat setup, corrupt/missing payload and manager refusal, rollback.'
Write-Output "Fixtures retained: $fixture"
