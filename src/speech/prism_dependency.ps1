Set-StrictMode -Version Latest
Add-Type -AssemblyName System.Web.Extensions
function Read-PrismJson([string]$Path) {
    $parser = New-Object Web.Script.Serialization.JavaScriptSerializer
    return $parser.DeserializeObject([IO.File]::ReadAllText($Path))
}
function Get-PrismHash([string]$Path) {
    $sha = [Security.Cryptography.SHA256]::Create(); $stream = $null
    try { $stream = [IO.File]::OpenRead($Path); return [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','').ToLowerInvariant() }
    finally { if ($stream) { $stream.Dispose() }; $sha.Dispose() }
}
function Assert-PrismAncestors([string]$Path) {
    $next = $Path
    while ($next) { Test-PrismReparse $next; $next = [IO.Path]::GetDirectoryName($next) }
}

function Get-PrismCanonicalPath([string]$Path) {
    if ([string]::IsNullOrWhiteSpace($Path) -or -not [IO.Path]::IsPathRooted($Path)) { throw 'An absolute path is required.' }
    return [IO.Path]::GetFullPath((Resolve-Path -LiteralPath $Path -ErrorAction Stop).Path).TrimEnd([IO.Path]::DirectorySeparatorChar)
}
function Test-PrismReparse([string]$Path) {
    if (Test-Path -LiteralPath $Path) { $item = Get-Item -LiteralPath $Path -Force; if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw "Reparse point is not allowed: $Path" } }
}
function Assert-PrismPlainTree([string]$Root, [string[]]$Names = @()) {
    Test-PrismReparse $Root
    foreach ($name in $Names) { if ([IO.Path]::GetFileName($name) -cne $name -or $name -in @('.', '..') -or $name.Contains(':')) { throw "Prism file names must be flat: $name" }; $p = Join-Path $Root $name; Test-PrismReparse $p }
}
function Get-PrismLock([string]$Path) {
    $lock = Read-PrismJson $Path
    if ($lock.schema -ne 1 -or !$lock.package_id -or !$lock.version -or !$lock.source_commit -or $lock.platform -cne 'windows' -or $lock.architecture -cne 'x86' -or !$lock.files -or $lock.files.Count -eq 0) { throw 'Invalid Prism lock metadata.' }
    foreach ($name in $lock.files.Keys) { $e = $lock.files[$name]; if ([IO.Path]::GetFileName($name) -cne $name -or $name -in @('.', '..') -or $name.Contains(':') -or $e.source -cne $name -or $e.sha256 -cnotmatch '^[0-9a-f]{64}$') { throw "Invalid Prism lock entry: $name" } }
    return $lock
}
function Get-PrismReceiptPath([string]$Game) { Join-Path $Game 'PrismRuntime\prism-install.json' }
function Assert-PrismInstalled([string]$GameDirectory, $Lock) {
    $game = Get-PrismCanonicalPath $GameDirectory; Assert-PrismAncestors $game
    $runtime = Join-Path $game 'PrismRuntime'; Test-PrismReparse $runtime
    $rp = Join-Path $runtime 'prism-install.json'; Test-PrismReparse $rp
    if (!(Test-Path -LiteralPath $rp -PathType Leaf)) { throw 'Prism prerequisite is not installed.' }
    $r = Read-PrismJson $rp
    $want = @{}; foreach ($n in $Lock.files.Keys) { $want[$n] = $Lock.files[$n].sha256 }
    if ($r.schema -ne 1 -or ![string]::Equals([IO.Path]::GetFullPath($r.game_directory), $game, [StringComparison]::OrdinalIgnoreCase) -or $r.platform -cne 'windows' -or $r.architecture -cne 'x86' -or $r.package_id -cne $Lock.package_id -or $r.version -cne $Lock.version -or $r.source_commit -cne $Lock.source_commit -or !$r.owners -or 'FFX-native' -notin $r.owners -or 'FFX-launcher' -notin $r.owners -or !$r.files -or $r.files.Count -ne $want.Count) { throw 'Prism receipt identity or file manifest does not match.' }
    foreach ($n in $want.Keys) { if (!$r.files.ContainsKey($n) -or $r.files[$n] -cne $want[$n]) { throw 'Prism receipt identity or file manifest does not match.' } }
    $expectedNames = @($Lock.files.Keys) + 'prism-install.json'
    $actualFiles = @(Get-ChildItem -LiteralPath $runtime -Force)
    if ($actualFiles.Count -ne $expectedNames.Count) { throw 'Prism runtime inventory does not match the source lock.' }
    foreach ($entry in $actualFiles) {
        if ($entry.PSIsContainer -or ($entry.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0 -or $entry.Name -notin $expectedNames) { throw 'Unexpected Prism runtime entry.' }
    }
    Assert-PrismPlainTree $runtime @($Lock.files.Keys)
    foreach ($n in $Lock.files.Keys) { $p = Join-Path $runtime $n; if (!(Test-Path -LiteralPath $p -PathType Leaf) -or (Get-PrismHash $p) -cne $Lock.files[$n].sha256) { throw "Prism file is missing or modified: $n" } }
    return $r
}

