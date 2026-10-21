param([Parameter(Mandatory=$true)][string]$TolkRoot)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..'))
$evidenceRoot = $env:FFX_EVIDENCE_ROOT
if ([string]::IsNullOrWhiteSpace($evidenceRoot)) { $evidenceRoot = Join-Path $repo '..\Analysis\FFX\RepositoryEvidence' }
$fixtureRoot = Join-Path ([IO.Path]::GetFullPath($evidenceRoot)) 'test-build'
$stage = Join-Path $repo 'install\native'
$fixture = Join-Path $fixtureRoot 'deployment-guard-tests'
[void][IO.Directory]::CreateDirectory($fixture)
$verify = Join-Path $repo 'src\proxy\verify_deployment.ps1'
$config = Join-Path $fixture 'FFX&X-2_LAUNCHER.exe.config'
$exe = Join-Path $fixture 'FFX.exe'
$proxy = Join-Path $fixture 'dinput8.dll'
# A real x86 executable, deliberately absent from the supported game hashes.
Copy-Item -LiteralPath (Join-Path $stage 'profile_preflight.exe') -Destination $exe -Force
[IO.File]::WriteAllText($proxy,'Preserve this installed proxy')
[IO.File]::WriteAllText($config,'<configuration/>')
$before = (Get-FileHash -LiteralPath $proxy).Hash
function Expect-Refusal([string]$Reason) {
    $refused = $false
    try { & $verify -GameDirectory $fixture -RepositoryRoot $repo -TolkRoot $TolkRoot }
    catch { $refused = $true; if ($_.Exception.Message -notlike "*$Reason*") { throw } }
    if (-not $refused) { throw "Deployment guard accepted $Reason" }
    if ((Get-FileHash -LiteralPath $proxy).Hash -ne $before) { throw 'Preflight changed the installed proxy.' }
}
Expect-Refusal 'Native executable validation failed'
[IO.File]::WriteAllText($config,'<configuration><runtime><appDomainManagerType value="Other.Manager"/></runtime></configuration>')
Expect-Refusal 'Launcher bootstrap configuration mismatch'
[IO.File]::WriteAllText($config,'<configuration/>')
$dependencyRoot = Join-Path $fixture 'incomplete-tolk'
[void][IO.Directory]::CreateDirectory((Join-Path $dependencyRoot 'dist_x86'))
[void][IO.Directory]::CreateDirectory((Join-Path $dependencyRoot 'libs\x86'))
Copy-Item -LiteralPath (Join-Path $TolkRoot 'dist_x86\Tolk.dll') -Destination (Join-Path $dependencyRoot 'dist_x86\Tolk.dll') -Force
Copy-Item -LiteralPath (Join-Path $TolkRoot 'libs\x86\nvdaControllerClient32.dll') -Destination (Join-Path $dependencyRoot 'libs\x86\nvdaControllerClient32.dll') -Force
$realTolk = $TolkRoot
$TolkRoot = $dependencyRoot
Expect-Refusal 'Required deployment artifact missing'
$TolkRoot = $realTolk
Write-Output 'Deployment guard checks passed: unsupported executable, launcher conflict and missing speech dependency refuse before installed files change.'
