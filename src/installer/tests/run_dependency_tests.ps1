param([Parameter(Mandatory=$true)][string]$InstallerPath)
$ErrorActionPreference = 'Stop'
$InstallerPath = [IO.Path]::GetFullPath($InstallerPath)
if ([IntPtr]::Size -ne 4 -or $PSVersionTable.PSEdition -eq 'Core') {
    $dependencyTestHost = Join-Path $env:WINDIR 'SysWOW64\WindowsPowerShell\v1.0\powershell.exe'
    & $dependencyTestHost -NoProfile -ExecutionPolicy Bypass -File $PSCommandPath -InstallerPath $InstallerPath
    if ($LASTEXITCODE -ne 0) { throw 'Dependency installer checks failed.' }
    return
}
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..'))
$work = [IO.Path]::GetFullPath((Join-Path $repo ('install\dependency-checks-' + [guid]::NewGuid().ToString('N'))))
if (-not $work.StartsWith($repo + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Fixture escapes workspace.' }
[void][IO.Directory]::CreateDirectory($work)
$test = Join-Path $work 'DependencyInstallerTests.exe'
$sources = @(Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot '..') -Filter '*.cs' -File | ForEach-Object { $_.FullName })
$compiler = Join-Path $env:WINDIR 'Microsoft.NET\Framework\v4.0.30319\csc.exe'
& $compiler /nologo /target:exe /platform:x86 /warnaserror+ /main:DependencyInstallerTests /reference:System.Windows.Forms.dll /reference:System.Drawing.dll /reference:System.Web.Extensions.dll ("/out:" + $test) (Join-Path $PSScriptRoot 'DependencyInstallerTests.cs') @sources
if ($LASTEXITCODE -ne 0) { throw 'Dependency test compilation failed.' }
$games = Join-Path $work 'games'
& $test $games $InstallerPath $repo
if ($LASTEXITCODE -ne 0) { throw 'Unified installer fixtures failed.' }
# Align internal deploy verification with the new installer's Windows identity.
# Only the fresh owned fixture is changed; the SDK source and live game are not used.
. (Join-Path $repo 'src\speech\prism_dependency.ps1')
$locked = Get-PrismLock (Join-Path $repo 'prism.lock.json')
$game = Join-Path $games 'fresh'
$receipt = Get-PrismReceiptPath $game
$parser = New-Object Web.Script.Serialization.JavaScriptSerializer
$assembly = [Reflection.Assembly]::LoadFile($InstallerPath)
$stream = $assembly.GetManifestResourceStream('Payload.prism.lock.json')
$memory = New-Object IO.MemoryStream
try { $stream.CopyTo($memory); $data = $parser.DeserializeObject([Text.Encoding]::UTF8.GetString($memory.ToArray())) }
finally { $stream.Dispose(); $memory.Dispose() }
$files = @{}; foreach ($name in $data.files.Keys) { $files[$name] = $data.files[$name].sha256 }
$data = @{ schema=1; game_directory=$game.ToUpperInvariant(); platform='windows'; architecture='x86'; package_id=$data.package_id; version=$data.version; source_commit=$data.source_commit; owners=@('FFX-native','FFX-launcher'); files=$files }
[IO.File]::WriteAllText($receipt, $parser.Serialize($data))
$receiptBytes = [IO.File]::ReadAllBytes($receipt)
$runtime = Join-Path $game 'PrismRuntime'
$before = @{}; foreach ($file in Get-ChildItem -LiteralPath $runtime -File) { $before[$file.Name] = (Get-PrismHash $file.FullName) + ':' + $file.LastWriteTimeUtc.Ticks }
$null = Assert-PrismInstalled $game $locked
foreach ($file in Get-ChildItem -LiteralPath $runtime -File) {
    if ($before[$file.Name] -cne ((Get-PrismHash $file.FullName) + ':' + $file.LastWriteTimeUtc.Ticks)) { throw 'Deploy verification wrote SDK files.' }
}
$extra = Join-Path $runtime 'unexpected.txt'
[IO.File]::WriteAllText($extra, 'foreign fixture')
$refused = $false
try { $null = Assert-PrismInstalled $game $locked } catch { $refused = $_.Exception.Message.Contains('inventory') }
if (-not $refused -or [IO.File]::ReadAllText($extra) -ne 'foreign fixture') { throw 'Deploy verifier accepted or changed an extra SDK file.' }
[IO.File]::Delete($extra)
$data.owners = @('FFX-native')
[IO.File]::WriteAllText($receipt, $parser.Serialize($data))
$refused = $false
try { $null = Assert-PrismInstalled $game $locked } catch { $refused = $_.Exception.Message.Contains('identity') }
if (-not $refused) { throw 'Deploy verifier accepted missing launcher ownership.' }
[IO.File]::WriteAllBytes($receipt, $receiptBytes)
$null = Assert-PrismInstalled $game $locked
Write-Output 'PASS: developer PRISM preflight accepts Windows path capitalization without SDK writes, refuses extra inventory and missing ownership.'
