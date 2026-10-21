[CmdletBinding()]
param(
    [ValidateSet('Editor', 'Validate', 'Package', 'Test', 'PerformanceTest', 'VRTest')]
    [string]$Action = 'Package',
    [string]$EnginePath = $env:UE_ROOT
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$Project = Join-Path $ProjectRoot 'MaterialHall.uproject'
if ([string]::IsNullOrWhiteSpace($EnginePath)) {
    $EnginePath = Join-Path $env:ProgramFiles 'Epic Games\UE_5.8'
}
$EnginePath = [IO.Path]::GetFullPath($EnginePath)
$Build = Join-Path $EnginePath 'Engine\Build\BatchFiles\Build.bat'
$UAT = Join-Path $EnginePath 'Engine\Build\BatchFiles\RunUAT.bat'
$EditorCmd = Join-Path $EnginePath 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
foreach ($Path in @($Project, $Build, $UAT, $EditorCmd)) {
    if (-not (Test-Path -LiteralPath $Path)) { throw "Required file missing: $Path. Pass -EnginePath for your Unreal install." }
}
$Version = Get-Content -Raw (Join-Path $EnginePath 'Engine\Build\Build.version') | ConvertFrom-Json
if ($Version.MajorVersion -ne 5 -or $Version.MinorVersion -ne 8 -or $Version.PatchVersion -lt 4) {
    throw 'Use Unreal Engine 5.8.4 (or a later 5.8 patch) for this project.'
}
function Invoke-Checked {
    param([string]$File, [string[]]$Arguments)
    & $File @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$File failed with exit code $LASTEXITCODE" }
}
function Build-Editor {
    Invoke-Checked $Build @('MaterialHallEditor', 'Win64', 'Development', "-Project=$Project", '-WaitMutex', '-NoHotReload', '-NoUBA')
}
switch ($Action) {
    'Editor' {
        Build-Editor
        Write-Host "Editor module built. Open $Project and press Play > Standalone Game."
    }
    'Validate' {
        Build-Editor
        Invoke-Checked $EditorCmd @($Project, '-run=pythonscript', "-script=$ProjectRoot\Tools\validate_scene.py", '-nullrhi', '-unattended', '-nosplash')
    }
    'Package' {
        Build-Editor
        Invoke-Checked $UAT @('BuildCookRun', "-project=$Project", '-noP4', '-platform=Win64',
            '-clientconfig=Development', '-build', '-cook', '-stage', '-pak', '-iostore',
            '-prereqs', '-map=/Game/MaterialHall/WhiteHall', '-unattended', '-utf8output',
            '-NoCompileEditor', '-UbtArgs=-NoUBA')
        $Executable = Join-Path $ProjectRoot 'Saved\StagedBuilds\Windows\MaterialHall.exe'
        if (-not (Test-Path -LiteralPath $Executable)) { throw "Expected packaged executable missing: $Executable" }
        Write-Host "Packaged game: $Executable"
        Write-Host 'Keep the whole Windows folder together; the EXE requires the adjacent content and runtime.'
    }
    { $_ -in @('Test', 'PerformanceTest', 'VRTest') } {
        $Executable = Join-Path $ProjectRoot 'Saved\StagedBuilds\Windows\MaterialHall.exe'
        if (-not (Test-Path -LiteralPath $Executable)) { throw 'Run -Action Package first.' }
        $CaptureDirectory = Join-Path $ProjectRoot ('Saved\Validation\Run-' + [Guid]::NewGuid().ToString('N'))
        New-Item -ItemType Directory -Path $CaptureDirectory | Out-Null
        $TestFlag = if ($Action -eq 'PerformanceTest') { '-HallPerformanceTest' } elseif ($Action -eq 'VRTest') { '-HallVRTest' } else { '-HallSelfTest' }
        $ReportName = if ($Action -eq 'PerformanceTest') { 'performance-check.json' } elseif ($Action -eq 'VRTest') { 'vr-check.json' } else { 'runtime-check.json' }
        $Arguments = '-nohmd -RenderOffscreen -windowed -ResX=1440 -ResY=900 -unattended -nosplash -nosound ' + $TestFlag + ' -HallCaptureDir="' + $CaptureDirectory + '"'
        Write-Host "Running $Action. Reports: $CaptureDirectory"
        $Process = Start-Process -FilePath $Executable -ArgumentList $Arguments -WindowStyle Hidden -Wait -PassThru
        if ($Process.ExitCode -ne 0) { throw "Runtime exited with code $($Process.ExitCode)" }
        $Report = Get-Content -Raw (Join-Path $CaptureDirectory $ReportName) | ConvertFrom-Json
        if (-not $Report.passed) { throw "Runtime verification failed; see $CaptureDirectory" }
        Write-Host "Runtime verification passed. Reports and rendered views: $CaptureDirectory"
    }
}
