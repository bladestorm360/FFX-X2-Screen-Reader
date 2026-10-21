# Read the shipped managed installer's payload as bytes; never execute it.
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..'))
$installer = Join-Path $repo 'releases/0.5.1-Test Build/ffx-x2-screen-reader-0.5.1-Test Build/InstallFFXScreenReader.exe'
$out = Join-Path $repo ('Docs/Scripts/Offline/output/shipped_threading_' + [DateTime]::UtcNow.ToString('yyyyMMdd_HHmmss_fffffff'))
[void][IO.Directory]::CreateDirectory($out)
$assembly = [Reflection.Assembly]::LoadFile($installer)
$stream = $assembly.GetManifestResourceStream('Payload.dinput8.dll')
if (-not $stream) { throw 'Shipped native payload missing.' }
$path = Join-Path $out 'dinput8.dll'
$file = [IO.File]::Create($path)
try { $stream.CopyTo($file) } finally { $file.Dispose(); $stream.Dispose() }
$hash = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
if ($hash -ne 'c9d561efe2dfec1846ac803532a766e65e01a404197d6bf08267294e7b34d5c8') { throw 'Shipped payload differs from its release evidence.' }
@{ installer=$installer; installer_sha256=(Get-FileHash -LiteralPath $installer -Algorithm SHA256).Hash.ToLowerInvariant(); native_sha256=$hash; output=$path; entry_point_executed=$false } |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $out 'manifest.json') -Encoding UTF8
Write-Output ('PASS: exact shipped 0.5.1 native payload retained for offline audit: ' + $out)
