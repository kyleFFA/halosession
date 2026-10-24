param(
    [string]$VcpkgRoot,
    [switch]$BuildSupportLibraries,
    [switch]$Rebuild
)
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path $PSScriptRoot -Parent
$taskVsWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
if (-not (Test-Path -LiteralPath $taskVsWhere)) { throw 'Install Visual Studio 2022 C++ Build Tools (v143) and a Windows SDK.' }
$taskVs = & $taskVsWhere -latest -products '*' -version '[17.0,18.0)' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $taskVs) { throw 'Visual Studio 2022 C++ tools were not found.' }
if (-not $VcpkgRoot) { $VcpkgRoot = $env:VCPKG_ROOT }
if (-not $VcpkgRoot) { $VcpkgRoot = Join-Path $taskVs 'VC/vcpkg' }
$taskTargets = Join-Path $VcpkgRoot 'scripts/buildsystems/msbuild/vcpkg.targets'
if (-not (Test-Path -LiteralPath $taskTargets)) { throw 'Supply -VcpkgRoot pointing to an installed/bootstrap-complete vcpkg checkout.' }
$taskBuild = Join-Path $taskVs 'MSBuild/Current/Bin/MSBuild.exe'
$taskAction = if ($Rebuild) { '/t:Rebuild' } else { '/t:Build' }
$taskProjects = @('HCMInternal')
if ($BuildSupportLibraries) { $taskProjects += @('HCMInterproc','HCMSpeedhack') }
Push-Location $taskRoot
$taskPreviousVs = $env:VSDIR
try {
    foreach ($taskProject in $taskProjects) {
        $taskArgs = @("$taskProject/$taskProject.vcxproj", $taskAction, '/m', '/p:Configuration=Release', '/p:Platform=x64',
            "/p:SolutionDir=$taskRoot/", '/p:PracticeBuildId=speed-overlay', '/p:VcpkgManifestInstall=true',
            "/p:VcpkgRoot=$VcpkgRoot/", "/p:ForceImportBeforeCppTargets=$taskTargets", '/verbosity:minimal', '/nologo',
            '/fl', "/flp:logfile=build-$taskProject.log;verbosity=normal")
        if ($taskProject -eq 'HCMSpeedhack') { $taskArgs += '/p:TargetName=PCMSpeedhack' }
        & $taskBuild @taskArgs
        if ($LASTEXITCODE -ne 0) { throw "$taskProject build failed; inspect its local build log." }
    }
    $env:VSDIR = $taskVs
    & cmd /c tools\build-launcher.cmd
    if ($LASTEXITCODE -ne 0) { throw 'Launcher build failed.' }
    $taskOutput = Join-Path $taskRoot 'HCMExternal/bin/x64/Release/net7.0-windows'
    $taskPointerData = Join-Path $taskRoot 'HCMInternal/InternalPointerData.xml'
    if (Test-Path -LiteralPath $taskPointerData) {
        Copy-Item -LiteralPath $taskPointerData -Destination $taskOutput
    } else {
        Write-Output 'Pointer XML is supplied separately with the compatible runtime; native compilation does not require it.'
    }
    Write-Output 'Source build completed. Session has not been launched. A complete player package still needs its compatible external runtime.'
} finally {
    $env:VSDIR = $taskPreviousVs
    Pop-Location
}
