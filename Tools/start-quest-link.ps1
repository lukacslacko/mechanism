[CmdletBinding()]
param([switch]$HardwareTest)
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$GameDirectory = Join-Path $ProjectRoot 'Saved\StagedBuilds\Windows'
$Executable = Join-Path $GameDirectory 'MaterialHall.exe'
if (-not (Test-Path -LiteralPath $Executable)) { throw 'Packaged app missing. Run Tools/build-windows.ps1 -Action Package first.' }
$Candidates = @(
    (Join-Path $env:ProgramFiles 'Meta Horizon\Support\oculus-runtime\oculus_openxr_64.json'),
    (Join-Path $env:ProgramFiles 'Oculus\Support\oculus-runtime\oculus_openxr_64.json')
)
$Install = Get-ItemProperty -LiteralPath 'HKLM:\SOFTWARE\WOW6432Node\Oculus VR, LLC\Oculus' -ErrorAction SilentlyContinue
if ($Install -and $Install.Base) { $Candidates += Join-Path $Install.Base 'Support\oculus-runtime\oculus_openxr_64.json' }
$Runtime = $Candidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if (-not $Runtime) { throw 'Install Meta Horizon Link on this PC, connect Quest by USB, and enter Quest Link in the headset first.' }
# Select Meta only for the child process. Do not change the global OpenXR
# runtime (which may be SteamVR) or persist an environment variable.
$PreviousRuntime = $env:XR_RUNTIME_JSON
try {
    $env:XR_RUNTIME_JSON = $Runtime
    $Arguments = '-vr -HallVR -windowed -ResX=1280 -ResY=720'
    if ($HardwareTest) {
        $ReportDirectory = Join-Path $ProjectRoot ('Saved\Validation\Quest-' + [Guid]::NewGuid().ToString('N'))
        New-Item -ItemType Directory -Path $ReportDirectory | Out-Null
        $Arguments += ' -HallVRHardwareTest -HallCaptureDir="' + $ReportDirectory + '"'
        Write-Host "Wear the headset and hold both controllers. The live test takes 45 seconds. Report: $ReportDirectory"
        $Process = Start-Process -FilePath $Executable -WorkingDirectory $GameDirectory -ArgumentList $Arguments -Wait -PassThru
        if ($Process.ExitCode -ne 0) { throw "Quest app exited with code $($Process.ExitCode)" }
        $Report = Get-Content -Raw (Join-Path $ReportDirectory 'vr-check.json') | ConvertFrom-Json
        $Report | Format-List
        if (-not $Report.passed) { throw "Headset verification failed; see $ReportDirectory" }
    } else {
        Start-Process -FilePath $Executable -WorkingDirectory $GameDirectory -ArgumentList $Arguments
    }
} finally { $env:XR_RUNTIME_JSON = $PreviousRuntime }
