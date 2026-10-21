param([string]$GameDirectory, [string]$SteamDirectory)
$ErrorActionPreference = 'Stop'

function Test-GameFolder([string]$Path) {
    return ((Test-Path -LiteralPath (Join-Path $Path 'FFX.exe') -PathType Leaf) -and
        (Test-Path -LiteralPath (Join-Path $Path 'FFX&X-2_LAUNCHER.exe') -PathType Leaf))
}

function Find-GameFolders {
    $roots = @()
    if ($SteamDirectory) { $roots += $SteamDirectory }
    else {
        foreach ($key in @('HKCU:\Software\Valve\Steam',
            'HKLM:\SOFTWARE\WOW6432Node\Valve\Steam', 'HKLM:\SOFTWARE\Valve\Steam')) {
            $properties = Get-ItemProperty -LiteralPath $key -ErrorAction SilentlyContinue
            if ($properties.SteamPath) { $roots += $properties.SteamPath }
            if ($properties.InstallPath) { $roots += $properties.InstallPath }
        }
        foreach ($base in @(${env:ProgramFiles(x86)}, $env:ProgramFiles)) {
            if ($base) { $roots += Join-Path $base 'Steam' }
        }
    }
    $libraries = @($roots)
    foreach ($root in ($roots | Select-Object -Unique)) {
        $vdf = Join-Path $root 'steamapps\libraryfolders.vdf'
        if (Test-Path -LiteralPath $vdf -PathType Leaf) {
            $contents = [IO.File]::ReadAllText($vdf)
            foreach ($match in [regex]::Matches($contents, '"(?:path|[0-9]+)"\s*"([^"\r\n]+)"')) {
                $path = $match.Groups[1].Value.Replace('\\', '\')
                if ([IO.Path]::IsPathRooted($path)) { $libraries += $path }
            }
        }
    }
    $found = @()
    foreach ($library in ($libraries | Select-Object -Unique)) {
        $apps = Join-Path $library 'steamapps'
        $manifest = Join-Path $apps 'appmanifest_359870.acf'
        $folder = 'FINAL FANTASY FFX&FFX-2 HD Remaster'
        if (Test-Path -LiteralPath $manifest -PathType Leaf) {
            $match = [regex]::Match([IO.File]::ReadAllText($manifest), '"installdir"\s*"([^"\r\n]+)"')
            if ($match.Success) { $folder = $match.Groups[1].Value }
        }
        if ($folder -in @('.', '..') -or $folder.IndexOfAny([IO.Path]::GetInvalidFileNameChars()) -ge 0) { continue }
        $candidate = Join-Path (Join-Path $apps 'common') $folder
        if (Test-GameFolder $candidate) { $found += [IO.Path]::GetFullPath($candidate) }
    }
    return $found | Select-Object -Unique
}

if ([string]::IsNullOrWhiteSpace($GameDirectory)) {
    $found = @(Find-GameFolders)
    if ($found.Count -eq 1) { $GameDirectory = $found[0] }
    elseif ($found.Count -gt 1) {
        for ($i = 0; $i -lt $found.Count; $i++) { Write-Output ("{0}. {1}" -f ($i + 1), $found[$i]) }
        $selection = 0
        $answer = Read-Host 'Enter the number of the game installation to update'
        if (-not [int]::TryParse($answer, [ref]$selection) -or $selection -lt 1 -or $selection -gt $found.Count) {
            throw 'Invalid installation number. Run setup again.'
        }
        $GameDirectory = $found[$selection - 1]
    }
    else {
        Write-Output 'Steam installation not found. Select the game folder in the folder browser.'
        Add-Type -AssemblyName System.Windows.Forms
        $picker = New-Object System.Windows.Forms.FolderBrowserDialog
        $picker.Description = 'Select the Final Fantasy X game folder containing FFX.exe'
        try {
            if ($picker.ShowDialog() -ne [System.Windows.Forms.DialogResult]::OK) { throw 'Installation cancelled.' }
            $GameDirectory = $picker.SelectedPath
        } finally { $picker.Dispose() }
    }
}
$game = [IO.Path]::GetFullPath($GameDirectory.Trim().Trim('"'))
if (-not (Test-GameFolder $game)) { throw 'Selected folder must contain FFX.exe and FFX&X-2_LAUNCHER.exe.' }
Write-Output "Installing screen reader to: $game"
foreach ($name in @('FFX', 'FFX-2', 'FFX-2_LM', 'FFX&X-2_Will', 'FFX&X-2_LAUNCHER')) {
    if (Get-Process -Name $name -ErrorAction SilentlyContinue) { throw 'Close the game and launcher before installation.' }
}

$staging = Join-Path $PSScriptRoot 'install'
$payload = @('dinput8.dll', 'MinHook.x86.dll', 'SDL3.dll', 'FFXLauncherReader.dll',
    'FFXLauncherNative.dll', 'Tolk.dll', 'nvdaControllerClient32.dll', 'SAAPI32.dll',
    'dolapi32.dll', 'FFX_SR_SDL3_LICENSE.txt', 'FFX_SR_Tolk_LICENSE.txt',
    'FFX_SR_NVDA_LICENSE.txt', 'FFX_SR_Native_Dependencies.md')
$configure = Join-Path $PSScriptRoot 'ConfigureLauncher.ps1'
$sums = Join-Path $PSScriptRoot 'SHA256SUMS.txt'
if (-not (Test-Path -LiteralPath $sums -PathType Leaf)) { throw 'Missing SHA256SUMS.txt. Extract the entire release ZIP first.' }
$hashes = @{}
foreach ($line in [IO.File]::ReadAllLines($sums)) {
    if ($line -match '^([a-fA-F0-9]{64})\s+(.+)$') { $hashes[$Matches[2].Replace('\', '/')] = $Matches[1] }
}
foreach ($relative in (@('ConfigureLauncher.ps1') + @($payload | ForEach-Object { 'install/' + $_ }))) {
    $source = Join-Path $PSScriptRoot $relative
    if (-not (Test-Path -LiteralPath $source -PathType Leaf) -or -not $hashes.ContainsKey($relative)) {
        throw "Incomplete release: $relative. Extract the entire release ZIP first."
    }
    if ((Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash -ne $hashes[$relative]) {
        throw "Release checksum failed: $relative. Extract a fresh copy of the ZIP."
    }
}

# Refuse a competing manager before copying DLLs or removing legacy files.
$configName = 'FFX&X-2_LAUNCHER.exe.config'
$config = Join-Path $game $configName
if (Test-Path -LiteralPath $config) {
    $xml = New-Object System.Xml.XmlDocument
    $xml.Load($config)
    if ($xml.DocumentElement.Name -ne 'configuration') { throw 'Unexpected launcher configuration root.' }
    foreach ($entry in @(@('appDomainManagerAssembly', 'FFXLauncherReader, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null'),
        @('appDomainManagerType', 'FfxLauncher.ReaderDomainManager'))) {
        $node = $xml.SelectSingleNode('/configuration/runtime/' + $entry[0])
        if ($node -and $node.GetAttribute('value') -ne $entry[1]) {
            throw 'Launcher already has another AppDomainManager; installation was left unchanged.'
        }
    }
}
$legacy = @('frida-gadget.dll', 'frida-gadget.config', 'frida-gadget.config.dll', 'ffx_sr.js')
$affected = @($payload) + @($configName, ($configName + '.ffx_sr_original'), ($configName + '.ffx_sr_pending')) + $legacy
$backup = Join-Path $game ('FFX_SR_Install_Backups\' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N'))
# Back up every replaced file first. Existing settings and unrelated mods stay intact.
[void][IO.Directory]::CreateDirectory($backup)
foreach ($name in $affected) {
    $target = Join-Path $game $name
    if (Test-Path -LiteralPath $target) {
        if (-not (Test-Path -LiteralPath $target -PathType Leaf)) { throw "Expected a file: $target" }
        Copy-Item -LiteralPath $target -Destination (Join-Path $backup $name)
    }
}
try {
    foreach ($name in $payload) {
        Copy-Item -LiteralPath (Join-Path $staging $name) -Destination (Join-Path $game $name) -Force
    }
    & $configure -GameDirectory $game -StagingDirectory $staging
    foreach ($name in $payload) {
        if ((Get-FileHash -LiteralPath (Join-Path $game $name) -Algorithm SHA256).Hash -ne $hashes['install/' + $name]) {
            throw "Installed file checksum failed: $name"
        }
    }
    foreach ($name in $legacy) {
        $target = Join-Path $game $name
        if (Test-Path -LiteralPath $target -PathType Leaf) {
            Remove-Item -LiteralPath $target
            Write-Output "Backed up and removed old mod file: $name"
        }
    }
} catch {
    $failure = $_
    foreach ($name in $affected) {
        $original = Join-Path $backup $name
        $target = Join-Path $game $name
        if (Test-Path -LiteralPath $original -PathType Leaf) { Copy-Item -LiteralPath $original -Destination $target -Force }
        elseif (Test-Path -LiteralPath $target -PathType Leaf) { Remove-Item -LiteralPath $target }
    }
    throw "Installation failed and previous files were restored. Backup: $backup. $failure"
}
Write-Output "Installation complete. Previous files are backed up in: $backup"
Write-Output 'Start your screen reader, then launch normally through Steam.'
