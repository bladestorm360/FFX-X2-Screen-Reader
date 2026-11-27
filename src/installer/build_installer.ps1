param([string]$OutputPath = (Join-Path $PSScriptRoot '..\..\install\windows-installer\InstallFFXScreenReader.exe'))
$ErrorActionPreference = 'Stop'
$output = [IO.Path]::GetFullPath($OutputPath)
if ([IntPtr]::Size -ne 4 -or $PSVersionTable.PSEdition -eq 'Core') {
    $hostPath = Join-Path $env:WINDIR 'SysWOW64\WindowsPowerShell\v1.0\powershell.exe'
    & $hostPath -NoProfile -ExecutionPolicy Bypass -File $PSCommandPath -OutputPath $output
    if ($LASTEXITCODE -ne 0) { throw 'x86 screen reader installer build failed.' }
    return
}
$compiler = Join-Path $env:WINDIR 'Microsoft.NET\Framework\v4.0.30319\csc.exe'
if (-not (Test-Path -LiteralPath $compiler -PathType Leaf)) { throw 'The Windows .NET Framework C# compiler is required.' }
$resources = foreach ($id in @('1235377778411650212', '5168047039601173807')) {
    '/resource:' + (Join-Path $PSScriptRoot ("baselines/359871_" + $id + ".txt")) + ',Baseline.' + $id
}
$folder = [IO.Path]::GetDirectoryName($output)
[void][IO.Directory]::CreateDirectory($folder)
$build = Join-Path $folder ('setup-build-' + [guid]::NewGuid().ToString('N'))
[void][IO.Directory]::CreateDirectory($build)
$temporary = Join-Path $build 'InstallFFXScreenReader.exe'
try {
    $arguments = @('/nologo', '/target:winexe', '/platform:x86', '/optimize+', '/warnaserror+',
        ('/out:' + $temporary), ('/win32manifest:' + (Join-Path $PSScriptRoot 'setup.manifest')),
        '/reference:System.Windows.Forms.dll', '/reference:System.Drawing.dll', '/reference:System.Web.Extensions.dll',
        '/reference:System.IO.Compression.dll') + $resources +
        @(Get-ChildItem -LiteralPath $PSScriptRoot -Filter '*.cs' | ForEach-Object { $_.FullName })
    & $compiler @arguments
    if ($LASTEXITCODE -ne 0) { throw 'Windows installer compilation failed.' }
    $assembly = [Reflection.Assembly]::Load([IO.File]::ReadAllBytes($temporary))
    $catalog = $assembly.GetType('FfxScreenReader.Setup.BaseGameCatalog', $true)
    $null = $catalog.GetMethod('Load').Invoke($null, @())
    if (@($assembly.GetManifestResourceNames()).Count -ne 2) { throw 'Setup must not embed release payload files.' }
    Copy-Item -LiteralPath $temporary -Destination $output -Force
    Write-Output "PASS: x86 GitHub setup with two pinned stock inventories and zero embedded payload files: $output"
} finally {
    if (Test-Path -LiteralPath $temporary) { [IO.File]::Delete($temporary) }
    [IO.Directory]::Delete($build)
}
