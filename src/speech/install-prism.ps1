[CmdletBinding()]
param([Parameter(Mandatory)][string]$GameDirectory,[Parameter(Mandatory)][string]$PackageDirectory,[string]$LockPath = (Join-Path $PSScriptRoot '..\..\prism.lock.json'),[switch]$Update)
. (Join-Path $PSScriptRoot 'prism_dependency.ps1')
function Get-Hash([string]$Path) { Get-PrismHash $Path }
function Test-FFXProcess {
    foreach ($n in @('FFX.exe','FFX-2.exe','FFX-2_LM.exe','FFX&X-2_Will.exe','FFX&X-2_LAUNCHER.exe')) { if (Get-Process -Name ([IO.Path]::GetFileNameWithoutExtension($n)) -ErrorAction SilentlyContinue) { throw "Close $n before installing Prism." } }
}
function Invoke-PrismCommit([string]$Stage,[string]$Destination,[string]$Backup,[bool]$HasOld) {
    if ($HasOld) { Move-Item -LiteralPath $Destination -Destination $Backup; try { Move-Item -LiteralPath $Stage -Destination $Destination } catch { Move-Item -LiteralPath $Backup -Destination $Destination; throw } }
    else { if (Test-Path -LiteralPath $Destination) { [IO.Directory]::Delete($Destination,$false) }; Move-Item -LiteralPath $Stage -Destination $Destination }
}
$game = Get-PrismCanonicalPath $GameDirectory; Assert-PrismAncestors $game
$package = Get-PrismCanonicalPath $PackageDirectory
$lock = Get-PrismLock $LockPath
$known = @('FFX.exe','FFX-2.exe','FFX-2_LM.exe','FFX&X-2_Will.exe','FFX&X-2_LAUNCHER.exe')
if (-not (Test-Path -LiteralPath $game -PathType Container)) { throw 'Game directory does not exist.' }
$gameExe = $false; foreach ($n in $known) { if (Test-Path -LiteralPath (Join-Path $game $n) -PathType Leaf) { $gameExe = $true } }; if (!$gameExe) { throw 'Game directory is not a recognized FFX installation.' }
$sha = [Security.Cryptography.SHA256]::Create()
try { $key = [BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($game.ToUpperInvariant()))).Replace('-','') } finally { $sha.Dispose() }
$mutexName = 'Local\PrismRuntime-' + $key
$mutex = [Threading.Mutex]::new($false,$mutexName); $held = $false
try {
    try { $held = $mutex.WaitOne([TimeSpan]::FromSeconds(30)) } catch [Threading.AbandonedMutexException] { $held = $true }; if (!$held) { throw 'Another Prism installation is active for this game directory.' }
    $targetParent = $game
    $dest = Join-Path $targetParent 'PrismRuntime'; $receiptPath = Join-Path $dest 'prism-install.json'
    Test-PrismReparse $targetParent; Test-PrismReparse $dest; Test-PrismReparse $receiptPath
    Test-FFXProcess
    Assert-PrismPlainTree $package @($lock.files.Keys)
    foreach ($n in $lock.files.Keys) { $src = Join-Path $package $lock.files[$n].source; if (!(Test-Path -LiteralPath $src -PathType Leaf) -or (Get-Hash $src) -cne $lock.files[$n].sha256) { throw "Source package verification failed: $n" } }
    if ((Test-Path -LiteralPath $dest -PathType Container) -and !(Test-Path -LiteralPath $receiptPath)) {
        $children = @(Get-ChildItem -LiteralPath $dest -Force); $matching = $children.Count -eq $lock.files.Count
        foreach ($n in $lock.files.Keys) { $f = Join-Path $dest $n; if (!(Test-Path -LiteralPath $f -PathType Leaf) -or (Get-Hash $f) -cne $lock.files[$n].sha256) { $matching = $false } }
        if ($matching) { $adopted = @{schema=1;game_directory=$game;platform='windows';architecture='x86';package_id=$lock.package_id;version=$lock.version;source_commit=$lock.source_commit;owners=@('FFX-native','FFX-launcher');files=@{}}; foreach ($n in $lock.files.Keys) { $adopted.files[$n] = $lock.files[$n].sha256 }; $adopted | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $receiptPath -Encoding utf8; $null = Assert-PrismInstalled $game $lock; 'Matching files adopted; receipt only'; return }
    }    $current = $null
    if (Test-Path -LiteralPath $receiptPath -PathType Leaf) { $current = Read-PrismJson $receiptPath; if ($current.schema -ne 1 -or $current.game_directory -cne $game -or $current.platform -cne 'windows' -or $current.architecture -cne 'x86' -or !$current.owners -or $current.files.Count -eq 0) { throw 'Existing Prism receipt is inconsistent; refusing repair.' }; Assert-PrismPlainTree $dest @($current.files.Keys); foreach ($n in $current.files.Keys) { $p = Join-Path $dest $n; if (!(Test-Path -LiteralPath $p -PathType Leaf) -or (Get-Hash $p) -cne $current.files[$n]) { throw "Existing owned Prism file is inconsistent: $n" } } }
    $wanted = @{}; foreach ($n in $lock.files.Keys) { $wanted[$n] = $lock.files[$n].sha256 }
    $same = $current -and $current.package_id -ceq $lock.package_id -and $current.version -ceq $lock.version -and $current.source_commit -ceq $lock.source_commit -and $current.files.Count -eq $wanted.Count
    if ($same) { foreach ($n in $wanted.Keys) { if (!$current.files.ContainsKey($n) -or $current.files[$n] -cne $wanted[$n]) { throw 'Same-package receipt manifest mismatch; refusing repair.' } }; $verified = Assert-PrismInstalled $game $lock; 'Already verified; zero writes'; return }
    if (Test-Path -LiteralPath $dest -PathType Container) {
        if (!$current) { $items = @(Get-ChildItem -LiteralPath $dest -Force); if ($items.Count -gt 0) { throw 'Unreceipted PrismRuntime exists; refusing adoption or overwrite.' } }
        elseif (!$Update) { throw 'A different consistent package is installed; pass -Update to replace it.' }
        elseif (@($current.owners | Where-Object { $_ -notin @('FFX-native','FFX-launcher') }).Count -gt 0) { throw 'Shared PrismRuntime has additional owners; incompatible replacement is refused.' }
    } elseif ($current) { throw 'Receipt exists without its owned runtime directory; refusing repair.' }
    $stage = Join-Path $targetParent ('.PrismRuntime.stage.' + [guid]::NewGuid().ToString('N')); $backup = Join-Path $targetParent ('.PrismRuntime.backup.' + [guid]::NewGuid().ToString('N'))
    foreach ($p in @($stage,$backup)) { $full = [IO.Path]::GetFullPath($p); if (!$full.StartsWith($targetParent + [IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Owned stage path escaped the game directory.' } }
    New-Item -ItemType Directory -Path $stage | Out-Null
    try {
        foreach ($n in $lock.files.Keys) { Copy-Item -LiteralPath (Join-Path $package $lock.files[$n].source) -Destination (Join-Path $stage $n); if ((Get-Hash (Join-Path $stage $n)) -cne $lock.files[$n].sha256) { throw "Staged Prism verification failed: $n" } }
        $owners = @('FFX-native','FFX-launcher'); if ($current) { $owners = @($current.owners | Select-Object -Unique); foreach ($o in @('FFX-native','FFX-launcher')) { if ($o -notin $owners) { $owners += $o } } }
        $receipt = @{schema=1;game_directory=$game;platform='windows';architecture='x86';package_id=$lock.package_id;version=$lock.version;source_commit=$lock.source_commit;owners=$owners;files=$wanted}
        $receipt | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $stage 'prism-install.json') -Encoding utf8
        Assert-PrismPlainTree $stage @($lock.files.Keys)
        foreach ($n in $lock.files.Keys) { if ((Get-Hash (Join-Path $stage $n)) -cne $wanted[$n]) { throw "Staged Prism verification failed: $n" } }
        Test-FFXProcess
        Invoke-PrismCommit $stage $dest $backup ([bool]$current)
        try { $null = Assert-PrismInstalled $game $lock } catch { if (Test-Path -LiteralPath $dest) { Remove-Item -LiteralPath $dest -Recurse -Force }; if ($current -and (Test-Path -LiteralPath $backup)) { Move-Item -LiteralPath $backup -Destination $dest }; throw }
        'Prism package installed and verified'
    } catch { if ($current -and (Test-Path -LiteralPath $backup) -and !(Test-Path -LiteralPath $dest)) { Move-Item -LiteralPath $backup -Destination $dest }; if ((Test-Path -LiteralPath $stage) -and [IO.Path]::GetFullPath($stage).StartsWith($targetParent + [IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { Remove-Item -LiteralPath $stage -Recurse -Force }; throw }
} finally { if ($held) { $mutex.ReleaseMutex() }; $mutex.Dispose() }




