$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..'))
$evidenceRoot = $env:FFX_EVIDENCE_ROOT
if ([string]::IsNullOrWhiteSpace($evidenceRoot)) { $evidenceRoot = Join-Path $repo '..\Analysis\FFX\RepositoryEvidence' }
$evidenceRoot = [IO.Path]::GetFullPath($evidenceRoot)
$testBuild = Join-Path $evidenceRoot 'test-build'
$staging = Join-Path $repo 'install\native'
$fixture = Join-Path $testBuild 'launcher-tests'
[void][IO.Directory]::CreateDirectory($fixture)
# A valid already-LAA surrogate exercises the real worker without patching.
$pe = New-Object byte[] 512
$pe[0]=0x4d;$pe[1]=0x5a;$pe[60]=64;$pe[64]=0x50;$pe[65]=0x45
$pe[68]=0x4c;$pe[69]=1;$pe[84]=0xe0;$pe[86]=0x22;$pe[87]=1;$pe[88]=0x0b;$pe[89]=1
[IO.File]::WriteAllBytes((Join-Path $fixture 'FFX.exe'),$pe)
$prismRuntime = Join-Path $fixture 'PrismRuntime'
[void][IO.Directory]::CreateDirectory($prismRuntime)
$installer = Join-Path $repo 'src\launcher\install.ps1'
$config = Join-Path $fixture 'FFX&X-2_LAUNCHER.exe.config'
$original = '<?xml version="1.0" encoding="utf-8"?><configuration><startup><supportedRuntime version="v4.0" sku=".NETFramework,Version=v4.0"/></startup><runtime><gcConcurrent enabled="true"/></runtime><appSettings><add key="Preserved" value="yes"/></appSettings></configuration>'
[IO.File]::WriteAllText($config, $original)
$backup = $config + '.ffx_sr_original'
# Only the fixed fixture filenames in the workspace are overwritten.
if (Test-Path -LiteralPath $backup) { Remove-Item -LiteralPath $backup }
& $installer -GameDirectory $fixture -StagingDirectory $staging
& $installer -GameDirectory $fixture -StagingDirectory $staging
$xml = New-Object Xml.XmlDocument; $xml.Load($config)
if ($xml.SelectNodes('/configuration/runtime/appDomainManagerType').Count -ne 1 -or
    $xml.SelectSingleNode('/configuration/appSettings/add').GetAttribute('value') -ne 'yes' -or
    $xml.SelectSingleNode('/configuration/runtime/gcConcurrent').GetAttribute('enabled') -ne 'true' -or
    [IO.File]::ReadAllText($backup) -ne $original) { throw 'Configuration merge / idempotency / original backup failed.' }
& $installer -GameDirectory $fixture -StagingDirectory $staging -Restore
if ([IO.File]::ReadAllText($config) -ne $original) { throw 'Restore failed.' }
[IO.File]::WriteAllText($config, '<configuration><runtime><appDomainManagerType value="Other.Manager"/></runtime></configuration>')
$before = [IO.File]::ReadAllBytes($config)
$rejected = $false
try { & $installer -GameDirectory $fixture -StagingDirectory $staging } catch { $rejected = $true }
if (-not $rejected -or [Convert]::ToBase64String($before) -ne [Convert]::ToBase64String([IO.File]::ReadAllBytes($config))) { throw 'Conflicting manager was overwritten.' }
# Configure the surrogate executable through the exact production installer.
[IO.File]::WriteAllText($config, $original)
& $installer -GameDirectory $fixture -StagingDirectory $staging
Copy-Item -LiteralPath $config -Destination (Join-Path $fixture 'launcher_fixture.exe.config') -Force
Copy-Item -LiteralPath (Join-Path $testBuild 'PrismRuntime\prism.dll') -Destination (Join-Path $prismRuntime 'prism.dll') -Force
$compiler = Join-Path $env:WINDIR 'Microsoft.NET\Framework\v4.0.30319\csc.exe'
& $compiler /nologo /platform:x86 /target:exe ("/out:" + (Join-Path $fixture 'launcher_fixture.exe')) /reference:System.Windows.Forms.dll /reference:System.Drawing.dll (Join-Path $PSScriptRoot 'launcher_fixture.cs')
if ($LASTEXITCODE -ne 0) { throw 'Launcher fixture compilation failed.' }
Push-Location $fixture
try { & .\launcher_fixture.exe; if ($LASTEXITCODE -ne 0) { throw 'Launcher fixture failed.' } } finally { Pop-Location }
Write-Output 'Launcher installer checks passed: preservation, idempotency, backup, conflict refusal, restore.'
