param([Parameter(Mandatory=$true)][string]$GameDirectory,
      [Parameter(Mandatory=$true)][string]$StagingDirectory,
      [switch]$Restore)
$ErrorActionPreference = 'Stop'
$game = [IO.Path]::GetFullPath($GameDirectory)
$config = Join-Path $game 'FFX&X-2_LAUNCHER.exe.config'
$backup = $config + '.ffx_sr_original'
if ($Restore) {
    if (-not (Test-Path -LiteralPath $backup)) { throw 'No original launcher configuration backup exists.' }
    Copy-Item -LiteralPath $backup -Destination $config -Force
    Write-Output 'Original launcher configuration restored.'
    exit 0
}
$document = New-Object System.Xml.XmlDocument
$document.PreserveWhitespace = $true
if (Test-Path -LiteralPath $config) { $document.Load($config) }
else { $document.LoadXml('<?xml version="1.0" encoding="utf-8"?><configuration/>') }
if ($document.DocumentElement.Name -ne 'configuration') { throw 'Unexpected launcher configuration root.' }
$runtime = $document.SelectSingleNode('/configuration/runtime')
if ($null -eq $runtime) {
    $runtime = $document.CreateElement('runtime')
    [void]$document.DocumentElement.AppendChild($runtime)
}
$assembly = 'FFXLauncherReader, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null'
$type = 'FfxLauncher.ReaderDomainManager'
foreach ($entry in @(@('appDomainManagerAssembly',$assembly), @('appDomainManagerType',$type))) {
    $node = $runtime.SelectSingleNode($entry[0])
    if ($null -ne $node -and $node.GetAttribute('value') -ne $entry[1]) {
        throw 'Launcher already has another AppDomainManager; configuration was preserved.'
    }
    if ($null -eq $node) { $node = $document.CreateElement($entry[0]); [void]$runtime.AppendChild($node) }
    $node.SetAttribute('value',$entry[1])
}
foreach ($name in @('FFXLauncherReader.dll','FFXLauncherNative.dll')) {
    $artifact = Join-Path $StagingDirectory $name
    if (-not (Test-Path -LiteralPath $artifact)) { throw "Missing built artifact: $name" }
}
# Prepare and parse the merged configuration before changing the original.
$temporary = $config + '.ffx_sr_pending'
$document.Save($temporary)
$verified = New-Object System.Xml.XmlDocument
$verified.Load($temporary)
if (-not (Test-Path -LiteralPath $backup)) {
    if (Test-Path -LiteralPath $config) { Copy-Item -LiteralPath $config -Destination $backup }
    else { [IO.File]::WriteAllText($backup,'<?xml version="1.0" encoding="utf-8"?><configuration/>') }
}
foreach ($name in @('FFXLauncherReader.dll','FFXLauncherNative.dll')) {
    Copy-Item -LiteralPath (Join-Path $StagingDirectory $name) -Destination (Join-Path $game $name) -Force
}
Move-Item -LiteralPath $temporary -Destination $config -Force
Write-Output 'Original launcher speech addon installed; configuration backup retained.'
