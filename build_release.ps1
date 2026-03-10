param(
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSCommandPath
$solutionPath = Join-Path $repoRoot 'src\Hello World!.sln'
$buildOutputDll = Join-Path $repoRoot 'src\x64\Release\d3d11.dll'
$releaseDir = Join-Path $repoRoot 'release'
$vswherePath = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'

if (-not (Test-Path $vswherePath)) {
    throw "vswhere.exe was not found. Install Visual Studio Build Tools first."
}

$instance = & $vswherePath -latest -products * -format json | ConvertFrom-Json | Select-Object -First 1
if (-not $instance) {
    throw "Visual Studio Build Tools were not found."
}

$vcvarsPath = Join-Path $instance.installationPath 'VC\Auxiliary\Build\vcvars64.bat'
$msbuildPath = Join-Path $instance.installationPath 'MSBuild\Current\Bin\MSBuild.exe'

if (-not (Test-Path $vcvarsPath)) {
    throw "vcvars64.bat was not found at $vcvarsPath"
}

if (-not (Test-Path $msbuildPath)) {
    throw "MSBuild.exe was not found at $msbuildPath"
}

if ($Clean -and (Test-Path $releaseDir)) {
    Remove-Item -Recurse -Force $releaseDir
}

function Invoke-SanitizedCmd {
    param(
        [string]$Arguments,
        [string]$WorkingDirectory
    )

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new($env:ComSpec, $Arguments)
    $startInfo.WorkingDirectory = $WorkingDirectory
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true

    foreach ($key in @($startInfo.Environment.Keys)) {
        $startInfo.Environment.Remove($key)
    }

    $mergedEnvironment = @{}
    foreach ($scope in 'Machine', 'User', 'Process') {
        foreach ($entry in [System.Environment]::GetEnvironmentVariables($scope).GetEnumerator()) {
            $name = [string]$entry.Key
            $mergedEnvironment[$name.ToLowerInvariant()] = @{
                Name = if ($name.Equals('PATH', [System.StringComparison]::OrdinalIgnoreCase)) { 'Path' } else { $name }
                Value = [string]$entry.Value
            }
        }
    }

    foreach ($entry in $mergedEnvironment.Values) {
        $startInfo.Environment[$entry.Name] = $entry.Value
    }

    $process = [System.Diagnostics.Process]::Start($startInfo)
    $stdout = $process.StandardOutput.ReadToEnd()
    $stderr = $process.StandardError.ReadToEnd()
    $process.WaitForExit()

    if ($stdout) {
        Write-Host $stdout.TrimEnd()
    }

    if ($stderr) {
        Write-Host $stderr.TrimEnd()
    }

    return $process.ExitCode
}

$buildCommand = "/d /c ""call `"$vcvarsPath`" && `"$msbuildPath`" `"$solutionPath`" /t:Build /p:Configuration=Release /p:Platform=x64"""
$exitCode = Invoke-SanitizedCmd -Arguments $buildCommand -WorkingDirectory (Join-Path $repoRoot 'src')

if ($exitCode -ne 0) {
    throw "Build failed with exit code $exitCode."
}

if (-not (Test-Path $buildOutputDll)) {
    throw "Build succeeded but d3d11.dll was not found at $buildOutputDll"
}

New-Item -ItemType Directory -Path $releaseDir -Force | Out-Null

Copy-Item -Force $buildOutputDll (Join-Path $releaseDir 'd3d11.dll')
Copy-Item -Force (Join-Path $repoRoot 'install.ps1') (Join-Path $releaseDir 'install.ps1')
Copy-Item -Force (Join-Path $repoRoot 'README.md') (Join-Path $releaseDir 'README.md')
Copy-Item -Force (Join-Path $repoRoot 'bo3_patch.example.ini') (Join-Path $releaseDir 'bo3_patch.example.ini')

Write-Host "Release files staged in $releaseDir"
